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

/// A frame is practically silent when its level (`frame_energy`) is under the note's median level
/// over its played frames (`0..fmax`, those holding data) by [`SILENT_REL_DB`] dB, or under
/// [`SILENT_FLOOR`] while the median is above it. Fixed: the data is shared by every voice.
pub const SILENT_REL_DB: f64 = -30.0;
/// `10^(SILENT_REL_DB / 20)`, written out so every port compares against the same number
pub const SILENT_REL: f64 = 0.031_622_776_601_683_8;
pub const SILENT_FLOOR: f64 = 1e-6;

/// Which of a note's frames are practically silent (see [`SILENT_REL_DB`]); frames past
/// `data_frames` (the zeros past the data) never are.
pub fn silent_frames(energy: &[f32], fmax: usize, data_frames: usize) -> Vec<bool> {
    let mut played: Vec<f64> = energy[..fmax.min(energy.len())]
        .iter()
        .filter(|&&e| e > 0.0)
        .map(|&e| e as f64)
        .collect();
    played.sort_by(f64::total_cmp);
    let k = played.len();
    let median = match k {
        0 => 0.0,
        _ if k % 2 == 1 => played[k / 2],
        _ => (played[k / 2 - 1] + played[k / 2]) / 2.0,
    };
    energy
        .iter()
        .enumerate()
        .map(|(f, &e)| {
            let e = e as f64;
            f < data_frames && (e < median * SILENT_REL || (e < SILENT_FLOOR && median > SILENT_FLOOR))
        })
        .collect()
}

/// A frame is misaligned when its partial slots stop following the note's partials: compared
/// with the note's reference frame (the played frame whose first slot is nearest the median
/// first-slot frequency), more than [`MISALIGNED_SHARE`] of the two frames' amplitude sits in
/// slots whose frequency differs by more than [`MISALIGNED_REL`]. Seen at the loop's end of
/// many S7 notes: a partial drops out of the analysis and the ones above it move down a slot,
/// so each slot's oscillator jumps, twice per loop (in, and back at the wrap). Skipped as
/// silent frames are, unless more than half the played frames are misaligned.
pub const MISALIGNED_REL: f64 = 0.10;
pub const MISALIGNED_SHARE: f64 = 0.5;

/// The share of amplitude, between two frames (freq, amp), in slots that moved by more than
/// [`MISALIGNED_REL`] (or are empty in one of them).
pub fn slot_moved_share(fa: &[f32], aa: &[f32], fb: &[f32], ab: &[f32]) -> f64 {
    let (mut moved, mut all) = (0.0, 0.0);
    for k in 0..fa.len().min(fb.len()) {
        let w = (aa[k] + ab[k]) as f64;
        if w <= 0.0 { continue; }
        all += w;
        let (x, y) = (fa[k] as f64, fb[k] as f64);
        if x <= 0.0 || y <= 0.0 || (x - y).abs() > MISALIGNED_REL * x.max(y) { moved += w; }
    }
    if all > 0.0 { moved / all } else { 0.0 }
}

pub fn misaligned_frames(freq: &[&[f32]], amp: &[&[f32]], fmax: usize, data_frames: usize) -> Vec<bool> {
    let n = freq.len();
    let lim = fmax.min(n);
    let mut f0: Vec<(f64, usize)> = (0..lim).filter(|&f| amp[f][0] > 0.0 && freq[f][0] > 0.0).map(|f| (freq[f][0] as f64, f)).collect();
    if f0.is_empty() { return vec![false; n]; }
    f0.sort_by(|a, b| a.0.total_cmp(&b.0));
    let r = f0[f0.len() / 2].1;
    let off: Vec<bool> = (0..n)
        .map(|f| f < data_frames && amp[f].iter().any(|&a| a > 0.0) && slot_moved_share(freq[r], amp[r], freq[f], amp[f]) > MISALIGNED_SHARE)
        .collect();
    if off[..lim].iter().filter(|&&b| b).count() * 2 > lim { vec![false; n] } else { off }
}

/// A frame's partials put back in the reference frame's slots: each reference slot takes the
/// frame's unused partial nearest its frequency (within [`MISALIGNED_REL`]), or stays empty;
/// partials left over go to the free slots above. What a slot's oscillator plays then follows
/// one partial from frame to frame.
pub fn realign(rf: &[f32], fa: &[f32], aa: &[f32]) -> ([f32; MAX_PARTIEL], [f32; MAX_PARTIEL]) {
    let (mut f, mut a) = ([0.0f32; MAX_PARTIEL], [0.0f32; MAX_PARTIEL]);
    let m = fa.len().min(MAX_PARTIEL);
    let mut used = [false; MAX_PARTIEL];
    let mut taken = [false; MAX_PARTIEL];
    for k in 0..rf.len().min(MAX_PARTIEL) {
        let r = rf[k] as f64;
        if r <= 0.0 { continue; }
        let best = (0..m)
            .filter(|&j| !used[j] && aa[j] > 0.0 && fa[j] > 0.0 && ((fa[j] as f64) - r).abs() <= MISALIGNED_REL * r)
            .min_by(|&x, &y| ((fa[x] as f64) - r).abs().total_cmp(&((fa[y] as f64) - r).abs()));
        if let Some(j) = best {
            used[j] = true;
            taken[k] = true;
            f[k] = fa[j];
            a[k] = aa[j];
        }
    }
    let mut free = (0..MAX_PARTIEL).filter(|&k| !taken[k] && rf.get(k).is_none_or(|&x| x <= 0.0));
    for j in 0..m {
        if !used[j] && aa[j] > 0.0 {
            if let Some(k) = free.next() { f[k] = fa[j]; a[k] = aa[j]; }
        }
    }
    (f, a)
}

/// Per frame of a note: its realigned partials ([`realign`]) when its slots don't follow the
/// reference frame's (see [`misaligned_frames`]) and realigning makes them; `None` otherwise.
/// Also which misaligned frames realigning can't save (skipped as silent ones are).
pub fn repair_note(freq: &[&[f32]], amp: &[&[f32]], fmax: usize, data_frames: usize)
    -> (Vec<Option<([f32; MAX_PARTIEL], [f32; MAX_PARTIEL])>>, Vec<bool>) {
    let n = freq.len();
    let off = misaligned_frames(freq, amp, fmax, data_frames);
    let mut out = vec![None; n];
    let mut lost = vec![false; n];
    let Some(r) = reference_frame(freq, amp, fmax) else { return (out, lost) };
    let lim = fmax.min(n);
    let need = (0..lim).filter(|&f| f < data_frames && slot_moved_share(freq[r], amp[r], freq[f], amp[f]) > REALIGN_SHARE).count();
    // nearly every frame off the reference: a note analysed that way (or a noisy one), not a
    // few frames gone astray; left as it is
    if need * 5 > lim * 4 { return (out, lost) }
    for f in 0..n {
        let share = slot_moved_share(freq[r], amp[r], freq[f], amp[f]);
        if f >= data_frames || share <= REALIGN_SHARE { continue; }
        let (nf, na) = realign(freq[r], freq[f], amp[f]);
        if slot_moved_share(freq[r], amp[r], &nf, &na) < share * 0.5 {
            out[f] = Some((nf, na));
        } else if off[f] {
            lost[f] = true;
        }
    }
    (out, lost)
}

/// A frame needs realigning past this share of moved amplitude ([`slot_moved_share`]).
pub const REALIGN_SHARE: f64 = 0.2;

fn reference_frame(freq: &[&[f32]], amp: &[&[f32]], fmax: usize) -> Option<usize> {
    let lim = fmax.min(freq.len());
    let mut f0: Vec<(f64, usize)> = (0..lim).filter(|&f| amp[f][0] > 0.0 && freq[f][0] > 0.0).map(|f| (freq[f][0] as f64, f)).collect();
    if f0.is_empty() { return None; }
    f0.sort_by(|a, b| a.0.total_cmp(&b.0));
    Some(f0[f0.len() / 2].1)
}

/// [`NoteInfo::skip_to`] from [`silent_frames`].
pub fn skip_table(silent: &[bool], fmax: usize, data_frames: usize) -> Vec<u16> {
    let n = silent.len();
    let fmax = fmax.min(n);
    (0..n)
        .map(|f| {
            let (lo, hi) = if f < fmax { (0, fmax) } else { (fmax, data_frames.max(f + 1).min(n)) };
            let len = hi - lo;
            (1..=len)
                .map(|k| lo + (f - lo + k - 1) % len)
                .find(|&g| !silent[g])
                .unwrap_or(f) as u16
        })
        .collect()
}

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
    /// per stored frame: the frame played in its place when silent frames are skipped (itself
    /// when it is not silent, else the next one that is not, wrapping within `0..fmax`, or within
    /// `fmax..data_frames` past it); see [`silent_frames`]
    pub skip_to: Vec<u16>,
}

pub struct DataSet {
    pub name: String,
    /// notes in the files (72, 73 or 80); the tables are indexed up to 80
    pub file_notes: usize,
    pub dure: [[f32; 3]; NOMBRE_DE_NOTE],
    pub info: Vec<NoteInfo>,
    set: siren_data::DataSet,
    /// realigned frames ([`repair_note`]): frame `MAX_TAB + i` of any note reads `repaired[i]`
    repaired: Vec<Box<([f32; MAX_PARTIEL], [f32; MAX_PARTIEL])>>,
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
        let mut repaired: Vec<Box<([f32; MAX_PARTIEL], [f32; MAX_PARTIEL])>> = Vec::new();
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
            let fq: Vec<&[f32]> = (0..frames).map(|f| set.frame_freq(n, f).unwrap_or(&ZERO_FRAME[..])).collect();
            let am: Vec<&[f32]> = (0..frames).map(|f| set.frame_amp(n, f).unwrap_or(&ZERO_FRAME[..])).collect();
            let (fixed, lost) = repair_note(&fq, &am, fmax, data_frames);
            let silent: Vec<bool> = silent_frames(&frame_energy, fmax, data_frames)
                .into_iter().zip(&lost).map(|(s, &m)| s || m).collect();
            let mut skip_to = skip_table(&silent, fmax, data_frames);
            // a realigned frame plays from the pool, past MAX_TAB
            let mut pool_of = vec![None; frames];
            for (f, r) in fixed.into_iter().enumerate() {
                if let Some(p) = r {
                    pool_of[f] = Some((MAX_TAB + repaired.len()) as u16);
                    repaired.push(Box::new(p));
                }
            }
            for t in skip_to.iter_mut() {
                if let Some(Some(p)) = pool_of.get(*t as usize) { *t = *p; }
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
                skip_to,
            });
        }
        DataSet { name: set.name().to_string(), file_notes: set.file_notes(), dure, info, set, repaired }
    }

    /// The loaded tables.
    pub fn source(&self) -> &siren_data::DataSet {
        &self.set
    }

    /// `tabFreq[note][frame]`, zeros where the plugin's arrays are zero
    #[inline]
    pub fn freq(&self, note: usize, frame: usize) -> &[f32; MAX_PARTIEL] {
        if frame >= MAX_TAB { return self.repaired.get(frame - MAX_TAB).map_or(&ZERO_FRAME, |r| &r.0); }
        self.set.frame_freq(note, frame).map_or(&ZERO_FRAME, |s| s.try_into().unwrap())
    }

    /// `tabAmp[note][frame]`
    #[inline]
    pub fn amp(&self, note: usize, frame: usize) -> &[f32; MAX_PARTIEL] {
        if frame >= MAX_TAB { return self.repaired.get(frame - MAX_TAB).map_or(&ZERO_FRAME, |r| &r.1); }
        self.set.frame_amp(note, frame).map_or(&ZERO_FRAME, |s| s.try_into().unwrap())
    }

    /// The frame played for `frame` of `note`: itself, or with `skip` the next frame that is not
    /// practically silent ([`NoteInfo::skip_to`]). No allocation: a table read.
    #[inline]
    pub fn play_frame(&self, note: usize, frame: usize, skip: bool) -> usize {
        if !skip {
            return frame;
        }
        self.info.get(note).and_then(|i| i.skip_to.get(frame)).map_or(frame, |&g| g as usize)
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
