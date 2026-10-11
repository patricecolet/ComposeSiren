//! The data sets as the DSP reads them, on top of `siren-data` (raw `Resources/` files or
//! compact `.sirendata`, the same values either way).
//!
//! Each data set (S1, S3, S4, S5, S7) holds, as the C++ tables do:
//! - `amp`, `freq`: `[note][MAX_TAB = 1000][MAX_PARTIEL = 200]`, zero past each note's data;
//! - `dure`: `[note][3]` = frame duration in samples, frames the plugin loops on (`fmax`),
//!   mean frequency;
//! - and, separately, the 392 rotor inertia values of `dataVectorIntervalSN`.

use std::io;
use std::path::Path;

pub use siren_data::{FRAMES as MAX_TAB, NOTES as NOMBRE_DE_NOTE, SLOTS as MAX_PARTIEL, VECTOR_INTERVAL as VECTOR_INTERVAL_LEN};

static ZERO_FRAME: [f32; MAX_PARTIEL] = [0.0; MAX_PARTIEL];

/// What is known about a note before any frame is fetched.
#[derive(Clone, Debug, Default)]
pub struct NoteInfo {
    pub index: usize,
    /// `dureTabs[n][0]`: samples per frame
    pub fdur: f32,
    /// `dureTabs[n][1]`: frames the plugin loops on (`countP` wraps here)
    pub fmax: usize,
    /// `dureTabs[n][2]`
    pub mean_freq: f32,
    /// frames stored (every later frame reads zeros)
    pub frames: usize,
    /// one past the last frame with a non-zero amplitude
    pub data_frames: usize,
    /// per stored frame: one past the last partial slot with a non-zero amplitude
    pub frame_partials: Vec<u16>,
    /// max of `frame_partials` over `0..fmax`
    pub max_partials: usize,
    /// max of `frame_partials` over every frame
    pub max_partials_all: usize,
    /// per stored frame: sqrt of the sum of squared amplitudes
    pub frame_energy: Vec<f32>,
}

pub struct DataSet {
    pub name: String,
    /// notes in the files (72, 73 or 80); the tables are indexed up to 80
    pub file_notes: usize,
    pub dure: [[f32; 3]; NOMBRE_DE_NOTE],
    pub info: Vec<NoteInfo>,
    set: siren_data::DataSet,
}

pub struct VectorInterval(pub [f32; VECTOR_INTERVAL_LEN]);

fn io_err(e: siren_data::Error) -> io::Error {
    match e {
        siren_data::Error::Io { source, .. } => source,
        e => io::Error::new(io::ErrorKind::InvalidData, e.to_string()),
    }
}

/// `dataVectorInterval{suffix}` of `dir` (the same file for raw and compact data).
pub fn read_vector_interval(dir: &Path, suffix: &str) -> io::Result<VectorInterval> {
    let bytes = std::fs::read(dir.join(format!("dataVectorInterval{suffix}")))?;
    let mut v = [0.0f32; VECTOR_INTERVAL_LEN];
    let (chunks, _) = bytes.as_chunks::<4>();
    for (d, c) in v.iter_mut().zip(chunks) {
        *d = f32::from_le_bytes(*c);
    }
    Ok(VectorInterval(v))
}

impl DataSet {
    /// `name`'s data in `dir`: `{name}.sirendata` if it is there, else the raw files.
    pub fn load(dir: &Path, name: &str) -> io::Result<DataSet> {
        let set = if dir.join(format!("{name}.sirendata")).exists() {
            siren_data::DataSet::load_compact(dir, name, name)
        } else {
            siren_data::DataSet::load_raw(dir, name, name)
        };
        Ok(Self::new(set.map_err(io_err)?))
    }

    pub fn new(set: siren_data::DataSet) -> DataSet {
        let mut dure = [[0.0f32; 3]; NOMBRE_DE_NOTE];
        let mut info = Vec::with_capacity(NOMBRE_DE_NOTE);
        for (n, d) in dure.iter_mut().enumerate() {
            *d = set.dure(n);
            let fmax = d[1].max(0.0) as usize;
            let frames = set.kept_frames(n);
            let mut frame_partials = Vec::with_capacity(frames);
            let mut frame_energy = Vec::with_capacity(frames);
            let mut data_frames = 0;
            for f in 0..frames {
                let a = set.frame_amp(n, f).unwrap_or(&ZERO_FRAME);
                let p = a.iter().rposition(|&x| x != 0.0).map_or(0, |p| p + 1);
                if p > 0 {
                    data_frames = f + 1;
                }
                frame_partials.push(p as u16);
                frame_energy.push(a.iter().map(|x| x * x).sum::<f32>().sqrt());
            }
            let max_of = |r: &[u16]| r.iter().copied().max().unwrap_or(0) as usize;
            info.push(NoteInfo {
                index: n,
                fdur: d[0],
                fmax,
                mean_freq: d[2],
                frames,
                data_frames,
                max_partials: max_of(&frame_partials[..fmax.min(frames)]),
                max_partials_all: max_of(&frame_partials),
                frame_partials,
                frame_energy,
            });
        }
        DataSet { name: set.name().to_string(), file_notes: set.file_notes(), dure, info, set }
    }

    /// The loaded tables.
    pub fn source(&self) -> &siren_data::DataSet {
        &self.set
    }

    /// `tabFreq[note][frame]`, zeros where the plugin's arrays are zero
    #[inline]
    pub fn freq(&self, note: usize, frame: usize) -> &[f32; MAX_PARTIEL] {
        self.set.frame_freq(note, frame).map_or(&ZERO_FRAME, |s| s.try_into().unwrap())
    }

    /// `tabAmp[note][frame]`
    #[inline]
    pub fn amp(&self, note: usize, frame: usize) -> &[f32; MAX_PARTIEL] {
        self.set.frame_amp(note, frame).map_or(&ZERO_FRAME, |s| s.try_into().unwrap())
    }

    /// Frames stored for `note`.
    pub fn frames(&self, note: usize) -> usize {
        self.set.kept_frames(note)
    }

    /// Bytes held for the frames, against the plugin's two `[80][1000][200]` arrays.
    pub fn bytes(&self) -> usize {
        self.set.stored_floats() * 4
    }
}
