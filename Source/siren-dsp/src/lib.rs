//! siren-dsp: ComposeSiren's synthesis, ported to Rust sample for sample, and a
//! study of how a Rust DSP could lay out and compute the same data.
//!
//! - [`data`]: the files of `Resources/`, kept frame by frame.
//! - [`sirene`]: the plugin's DSP, ported sample-exact, plus the lab's choice
//!   of frames.
//! - [`layout`], [`kernels`]: a cache-friendly layout of the same data and
//!   faster ways to sum the partials (the kernel variants).

pub mod data;
pub mod ffi;
pub mod kernels;
pub mod layout;
pub mod midi_in;
pub mod siren;
pub mod sirene;

use std::collections::HashMap;
use std::io;
use std::path::{Path, PathBuf};

/// The data files: `$COMPOSESIREN_RESOURCES`, else this repository's `Resources/`.
pub fn default_resources() -> PathBuf {
    if let Some(p) = std::env::var_os("COMPOSESIREN_RESOURCES") {
        return p.into();
    }
    // this crate: <ComposeSiren>/Source/siren-dsp
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../../Resources")
}
use std::sync::{Arc, Mutex};

use data::{DataSet, VectorInterval};
use siren::{SirenDef, SirenId};
use sirene::{FrameControl, FrameSel, LabFrames, Voice};

/// The data sets of one `Resources/` directory, loaded on first use. A library
/// seeded with [`Library::mem`] has no directory: what it answers with was put
/// there by [`Library::put_data`].
pub struct Library {
    dir: Option<PathBuf>,
    sets: Mutex<HashMap<String, Arc<DataSet>>>,
    intervals: Mutex<HashMap<String, Arc<VectorInterval>>>,
}

impl Library {
    pub fn new(dir: impl Into<PathBuf>) -> Library {
        Library { dir: Some(dir.into()), sets: Mutex::default(), intervals: Mutex::default() }
    }

    /// A library that holds no `Resources/` and so loads nothing: every data
    /// set it can answer for has to be seeded first (for hosts without the files,
    /// a browser for instance).
    pub fn mem() -> Library {
        Library { dir: None, sets: Mutex::default(), intervals: Mutex::default() }
    }

    pub fn dir(&self) -> Option<&Path> {
        self.dir.as_deref()
    }

    /// Seeds a data set built in memory, under the name the sirens read it by
    /// (`"S1"`, `"S3"`, …). Replaces anything already under that name.
    pub fn put_data(&self, name: &str, set: Arc<DataSet>) {
        self.sets.lock().unwrap().insert(name.to_string(), set);
    }

    /// Seeds a vector interval, under the name in `SirenDef::vector_interval`.
    pub fn put_vector_interval(&self, name: &str, vi: Arc<VectorInterval>) {
        self.intervals.lock().unwrap().insert(name.to_string(), vi);
    }

    pub fn data(&self, def: &SirenDef) -> io::Result<Arc<DataSet>> {
        if let Some(s) = self.sets.lock().unwrap().get(def.data) {
            return Ok(s.clone());
        }
        let dir = self
            .dir()
            .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, format!("{}: not seeded", def.data)))?;
        // loaded outside the lock: two first requests may both load, once
        let set = Arc::new(DataSet::load(dir, def.data)?);
        Ok(self.sets.lock().unwrap().entry(def.data.to_string()).or_insert(set).clone())
    }

    pub fn vector_interval(&self, def: &SirenDef) -> io::Result<Arc<VectorInterval>> {
        if let Some(v) = self.intervals.lock().unwrap().get(def.vector_interval) {
            return Ok(v.clone());
        }
        let dir = self.dir().ok_or_else(|| {
            io::Error::new(io::ErrorKind::NotFound, format!("{}: not seeded", def.vector_interval))
        })?;
        let v = Arc::new(data::read_vector_interval(dir, def.vector_interval)?);
        Ok(self.intervals.lock().unwrap().entry(def.vector_interval.to_string()).or_insert(v).clone())
    }

    pub fn voice(&self, id: SirenId, sample_rate: f64) -> io::Result<Voice> {
        let def = id.def();
        let vi = self.vector_interval(def)?;
        Ok(Voice::new(def, self.data(def)?, &vi, sample_rate))
    }
}

/// How the lab picks frames for one render.
#[derive(Clone, Debug)]
pub enum Frames {
    /// the plugin's behaviour (frame 0, reset every millisecond)
    Legacy,
    /// `[start, end)` for the rendered note, full loops for the others
    Loop { start: usize, end: usize },
    Freeze(usize),
}

#[derive(Clone, Debug)]
pub struct RenderSpec {
    pub siren: SirenId,
    /// data note index: it sounds `meanFreq` of that note. The plugin's MIDI
    /// note for it is `note + 12`.
    pub note: usize,
    /// cents above the note (0..99): with crossfade, between two notes' data
    pub cents: i32,
    pub velocity: i32,
    pub seconds: f64,
    /// note held for `seconds`, then this much tail to fade out
    pub release_seconds: f64,
    pub sample_rate: f64,
    pub partials: i32,
    pub crossfade: bool,
    pub bit16: bool,
    pub glide: bool,
    pub frames: Frames,
    pub speed: f32,
    pub interpolate: bool,
    pub smoothing: f64,
}

impl Default for RenderSpec {
    fn default() -> Self {
        RenderSpec {
            siren: SirenId::S1,
            note: 60,
            cents: 0,
            velocity: 127,
            seconds: 2.0,
            release_seconds: 0.3,
            sample_rate: 44100.0,
            partials: 30,
            crossfade: false,
            bit16: false,
            glide: false,
            frames: Frames::Legacy,
            speed: 1.0,
            interpolate: false,
            smoothing: 0.001,
        }
    }
}

/// Renders one note with the ported DSP. Returns mono samples.
pub fn render(lib: &Library, spec: &RenderSpec) -> io::Result<Vec<f32>> {
    let mut v = lib.voice(spec.siren, spec.sample_rate)?;
    let s = &mut v.siren;
    s.change_qualite(spec.partials);
    s.set_crossfade(spec.crossfade);
    s.set_16ou8bit(!spec.bit16);
    if (spec.smoothing - 0.001).abs() > 1e-12 {
        s.set_smoothing((spec.smoothing, 1.0 - spec.smoothing));
    }
    s.frames = match spec.frames {
        Frames::Legacy => FrameControl::Legacy,
        Frames::Loop { start, end } => {
            let mut l = LabFrames::full_loops(s.data());
            l.sel[spec.note] = FrameSel { start, end, freeze: None };
            l.speed = spec.speed;
            l.interpolate = spec.interpolate;
            FrameControl::Lab(l)
        }
        Frames::Freeze(k) => {
            let mut l = LabFrames::full_loops(s.data());
            l.sel[spec.note].freeze = Some(k);
            l.speed = spec.speed;
            l.interpolate = spec.interpolate;
            FrameControl::Lab(l)
        }
    };
    v.glide = spec.glide;

    let held = (spec.seconds * spec.sample_rate) as usize;
    let tail = (spec.release_seconds * spec.sample_rate) as usize;
    let mut out = Vec::with_capacity(held + tail);
    let midi_note = spec.note as i32 + 12;
    v.note_on(midi_note, spec.velocity);
    if spec.cents != 0 {
        // the MIDI path only gives whole notes: set the pitch directly
        v.glide = false;
        v.siren.jump_to(spec.note as i32 * 100 + spec.cents);
    }
    for _ in 0..held {
        out.push(v.process());
    }
    v.siren.silence();
    for _ in 0..tail {
        out.push(v.process());
    }
    Ok(out)
}
