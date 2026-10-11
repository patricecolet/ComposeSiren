//! ComposeSiren's siren data sets, loaded as the C++ DSP (`Sirene::readDataFromBinaryFile`)
//! sees them: `[80][1000][200]` amplitude and frequency tables, `[80][3]` note durations
//! (`fdur`, `fmax`, `meanFreq`) and 392 rotor inertia rates, zero wherever the files are shorter.
//!
//! Two sources, same values:
//! - raw: `dataAmpSN`, `dataFreqSN`, `datadureTabsSN`, `dataVectorIntervalSN` in a directory
//!   (`Resources/` or the build's `Resources-processed/`);
//! - compact: `SN.sirendata` (see `ResourcesProcessing/compact-format.md`) plus the same
//!   `dataVectorIntervalSN` file.
//!
//! Storage keeps only each note's frames up to its last non-zero one (about 57 MB for all data
//! sets instead of 603 MB); every read past them is `0.0`, as in the C++ tables. No allocation
//! after loading: the accessors are safe on the audio thread.

use std::fmt;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};

/// Notes in the C++ tables (`NOMBRE_DE_NOTE`).
pub const NOTES: usize = 80;
/// Frames per note in the raw tables (`MAX_TAB`).
pub const FRAMES: usize = 1000;
/// Partial slots per frame (`MAX_Partiel`).
pub const SLOTS: usize = 200;
/// Rotor inertia rates per data set.
pub const VECTOR_INTERVAL: usize = 392;

const MAGIC: &[u8; 4] = b"CSSD";
const VERSION: u32 = 1;

/// The data set files ComposeSiren ships (S2 plays S1's, S6 plays S5's).
pub const DATA_SETS: [&str; 5] = ["S1", "S3", "S4", "S5", "S7"];

#[derive(Debug)]
pub enum Error {
    Io { path: PathBuf, source: io::Error },
    Format { what: String, reason: String },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::Io { path, source } => write!(f, "{}: {source}", path.display()),
            Error::Format { what, reason } => write!(f, "{what}: {reason}"),
        }
    }
}

impl std::error::Error for Error {}

fn format_err(what: &str, reason: impl Into<String>) -> Error {
    Error::Format { what: what.to_string(), reason: reason.into() }
}

fn read(path: &Path) -> Result<Vec<u8>, Error> {
    fs::read(path).map_err(|source| Error::Io { path: path.to_path_buf(), source })
}

fn f32s(bytes: &[u8]) -> impl Iterator<Item = f32> + '_ {
    bytes.as_chunks::<4>().0.iter().map(|c| f32::from_le_bytes([c[0], c[1], c[2], c[3]]))
}

/// One note: its stored frames, amp then freq, each `kept * SLOTS` floats.
#[derive(Clone, Default)]
struct Note {
    kept: usize,
    amp: Vec<f32>,
    freq: Vec<f32>,
}

/// One data set, as the C++ tables hold it.
#[derive(Clone)]
pub struct DataSet {
    name: String,
    file_notes: usize,
    notes: Vec<Note>, // NOTES entries; past file_notes, empty
    dure: [[f32; 3]; NOTES],
    vector_interval: [f32; VECTOR_INTERVAL],
}

impl fmt::Debug for DataSet {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DataSet")
            .field("name", &self.name)
            .field("file_notes", &self.file_notes)
            .field("stored_floats", &self.stored_floats())
            .finish()
    }
}

/// The C++ reads at most the table's size from each file and leaves the rest zero.
fn dure_table(floats: impl Iterator<Item = f32>) -> [[f32; 3]; NOTES] {
    let mut t = [[0.0; 3]; NOTES];
    for (i, x) in floats.take(NOTES * 3).enumerate() {
        t[i / 3][i % 3] = x;
    }
    t
}

fn vector_interval(dir: &Path, name: &str) -> Result<[f32; VECTOR_INTERVAL], Error> {
    let bytes = read(&dir.join(format!("dataVectorInterval{name}")))?;
    let mut v = [0.0; VECTOR_INTERVAL];
    for (i, x) in f32s(&bytes).take(VECTOR_INTERVAL).enumerate() {
        v[i] = x;
    }
    Ok(v)
}

/// Frames up to the last one with any non-zero byte in amp or freq (as the F# packer).
fn kept_frames(amp: &[u8], freq: &[u8]) -> usize {
    let frame = SLOTS * 4;
    (0..FRAMES)
        .rev()
        .find(|f| {
            let r = f * frame..(f + 1) * frame;
            amp[r.clone()].iter().any(|&b| b != 0) || freq[r].iter().any(|&b| b != 0)
        })
        .map_or(0, |f| f + 1)
}

impl DataSet {
    /// The raw files `dataAmp{name}`, `dataFreq{name}`, `datadureTabs{name}` and
    /// `dataVectorInterval{vector_interval}` of `dir` (S2 uses S1's data with its own name in
    /// the C++ table; pass the file names the plugin uses).
    pub fn load_raw(dir: &Path, name: &str, vector_interval_name: &str) -> Result<DataSet, Error> {
        let amp = read(&dir.join(format!("dataAmp{name}")))?;
        let freq = read(&dir.join(format!("dataFreq{name}")))?;
        let dure = read(&dir.join(format!("datadureTabs{name}")))?;
        let note_bytes = FRAMES * SLOTS * 4;
        // like the C++: read up to the table, zero past the file's end (whole notes only here)
        if amp.len() % note_bytes != 0 || freq.len() % note_bytes != 0 {
            return Err(format_err(name, "dataAmp/dataFreq are not whole [note][1000][200] f32 tables"));
        }
        let file_notes = (amp.len() / note_bytes).max(freq.len() / note_bytes);
        let mut notes = vec![Note::default(); NOTES];
        for (n, note) in notes.iter_mut().enumerate().take(file_notes.min(NOTES)) {
            let slice = |b: &[u8]| -> Vec<u8> {
                let r = n * note_bytes..(n + 1) * note_bytes;
                b.get(r).map_or_else(|| vec![0; note_bytes], <[u8]>::to_vec)
            };
            let (a, q) = (slice(&amp), slice(&freq));
            let kept = kept_frames(&a, &q);
            note.kept = kept;
            note.amp = f32s(&a[..kept * SLOTS * 4]).collect();
            note.freq = f32s(&q[..kept * SLOTS * 4]).collect();
        }
        Ok(DataSet {
            name: name.to_string(),
            file_notes,
            notes,
            dure: dure_table(f32s(&dure)),
            vector_interval: vector_interval(dir, vector_interval_name)?,
        })
    }

    /// `{name}.sirendata` and `dataVectorInterval{vector_interval_name}` of `dir`.
    pub fn load_compact(dir: &Path, name: &str, vector_interval_name: &str) -> Result<DataSet, Error> {
        let bytes = read(&dir.join(format!("{name}.sirendata")))?;
        let mut set = Self::from_compact(name, &bytes)?;
        set.vector_interval = vector_interval(dir, vector_interval_name)?;
        Ok(set)
    }

    /// A data set built from memory: `dure` (up to `NOTES * 3` floats), each of the first
    /// `file_notes` notes' kept frame count in `kept`, and `frames` holding, note after note,
    /// its kept frames' amp then freq (`kept * SLOTS` little-endian f32 each): a compact file's
    /// body without its header, for a host with no filesystem (a web page, say).
    pub fn from_frames(
        name: &str,
        dure: &[f32],
        kept: &[usize],
        file_notes: usize,
        frames: &[u8],
        vector_interval: [f32; VECTOR_INTERVAL],
    ) -> Result<DataSet, Error> {
        if kept.len() < file_notes || file_notes > NOTES {
            return Err(format_err(name, format!("{file_notes} notes, {} kept counts, at most {NOTES}", kept.len())));
        }
        let mut notes = vec![Note::default(); NOTES];
        let mut pos = 0;
        for (n, &k) in kept.iter().take(file_notes).enumerate() {
            if k > FRAMES {
                return Err(format_err(name, format!("note {n}: {k} frames")));
            }
            let len = k * SLOTS * 4;
            let mut take = || -> Result<Vec<f32>, Error> {
                let b = frames.get(pos..pos + len).ok_or_else(|| format_err(name, format!("frames end in note {n}")))?;
                pos += len;
                Ok(f32s(b).collect())
            };
            let (amp, freq) = (take()?, take()?);
            notes[n] = Note { kept: k, amp, freq };
        }
        if pos != frames.len() {
            return Err(format_err(name, "trailing bytes after the last note"));
        }
        Ok(DataSet { name: name.to_string(), file_notes, notes, dure: dure_table(dure.iter().copied()), vector_interval })
    }

    /// A compact file's contents (the vector interval rates stay zero: they are a separate file).
    pub fn from_compact(name: &str, bytes: &[u8]) -> Result<DataSet, Error> {
        if bytes.len() < 28 || &bytes[..4] != MAGIC {
            return Err(format_err(name, "not a compact siren data file (magic)"));
        }
        let (body, crc) = bytes.split_at(bytes.len() - 4);
        if crc32(body) != u32::from_le_bytes([crc[0], crc[1], crc[2], crc[3]]) {
            return Err(format_err(name, "CRC mismatch, the file is damaged"));
        }
        let mut pos = 4;
        let mut u32_at = |what: &str| -> Result<u32, Error> {
            let b = body.get(pos..pos + 4).ok_or_else(|| format_err(name, format!("truncated at {what}")))?;
            pos += 4;
            Ok(u32::from_le_bytes([b[0], b[1], b[2], b[3]]))
        };
        let version = u32_at("version")?;
        if version != VERSION {
            return Err(format_err(name, format!("compact format version {version}, this reader knows {VERSION}")));
        }
        let (slots, frames) = (u32_at("slots")? as usize, u32_at("frames")? as usize);
        if slots != SLOTS || frames != FRAMES {
            return Err(format_err(name, format!("{frames} x {slots} tables, expected {FRAMES} x {SLOTS}")));
        }
        let file_notes = u32_at("notes")? as usize;
        let dure_floats = u32_at("dure count")? as usize;
        let mut take = |len: usize, what: &str| -> Result<&[u8], Error> {
            let b = body.get(pos..pos + len).ok_or_else(|| format_err(name, format!("truncated at {what}")))?;
            pos += len;
            Ok(b)
        };
        let dure = dure_table(f32s(take(dure_floats * 4, "dure")?));
        let kept: Vec<usize> = take(file_notes * 4, "kept")?
            .as_chunks::<4>().0.iter()
            .map(|c| u32::from_le_bytes([c[0], c[1], c[2], c[3]]) as usize)
            .collect();
        let mut notes = vec![Note::default(); NOTES];
        for (n, &k) in kept.iter().enumerate() {
            if k > FRAMES {
                return Err(format_err(name, format!("note {n}: {k} frames")));
            }
            let amp: Vec<f32> = f32s(take(k * SLOTS * 4, "amp")?).collect();
            let freq: Vec<f32> = f32s(take(k * SLOTS * 4, "freq")?).collect();
            if n < NOTES {
                notes[n] = Note { kept: k, amp, freq };
            }
        }
        if pos != body.len() {
            return Err(format_err(name, "trailing bytes after the last note"));
        }
        Ok(DataSet { name: name.to_string(), file_notes, notes, dure, vector_interval: [0.0; VECTOR_INTERVAL] })
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    /// Notes the data files hold (72, 73 or 80); the tables have [`NOTES`].
    pub fn file_notes(&self) -> usize {
        self.file_notes
    }

    /// Frames stored for `note`: every frame from there on reads 0.
    pub fn kept_frames(&self, note: usize) -> usize {
        self.notes.get(note).map_or(0, |n| n.kept)
    }

    /// `tabAmp[note][frame][slot]`.
    #[inline]
    pub fn amp(&self, note: usize, frame: usize, slot: usize) -> f32 {
        self.frame_amp(note, frame).map_or(0.0, |f| f[slot])
    }

    /// `tabFreq[note][frame][slot]`.
    #[inline]
    pub fn freq(&self, note: usize, frame: usize, slot: usize) -> f32 {
        self.frame_freq(note, frame).map_or(0.0, |f| f[slot])
    }

    /// The 200 amplitudes of a frame, `None` where the C++ table holds zeros.
    #[inline]
    pub fn frame_amp(&self, note: usize, frame: usize) -> Option<&[f32]> {
        let n = self.notes.get(note)?;
        (frame < n.kept).then(|| &n.amp[frame * SLOTS..(frame + 1) * SLOTS])
    }

    /// The 200 frequencies of a frame, `None` where the C++ table holds zeros.
    #[inline]
    pub fn frame_freq(&self, note: usize, frame: usize) -> Option<&[f32]> {
        let n = self.notes.get(note)?;
        (frame < n.kept).then(|| &n.freq[frame * SLOTS..(frame + 1) * SLOTS])
    }

    /// `dureTabs[note]`: samples per frame, frames looped, mean frequency.
    pub fn dure(&self, note: usize) -> [f32; 3] {
        self.dure.get(note).copied().unwrap_or([0.0; 3])
    }

    /// `vectorInterval`.
    pub fn vector_interval(&self) -> &[f32; VECTOR_INTERVAL] {
        &self.vector_interval
    }

    /// Floats held in memory for amp and freq.
    pub fn stored_floats(&self) -> usize {
        self.notes.iter().map(|n| n.amp.len() + n.freq.len()).sum()
    }
}

/// CRC-32 (IEEE 802.3, as zlib).
pub fn crc32(bytes: &[u8]) -> u32 {
    const TABLE: [u32; 256] = {
        let mut t = [0u32; 256];
        let mut n = 0;
        while n < 256 {
            let mut c = n as u32;
            let mut k = 0;
            while k < 8 {
                c = if c & 1 != 0 { 0xEDB8_8320 ^ (c >> 1) } else { c >> 1 };
                k += 1;
            }
            t[n] = c;
            n += 1;
        }
        t
    };
    let mut c = 0xFFFF_FFFFu32;
    for &b in bytes {
        c = TABLE[((c ^ b as u32) & 0xFF) as usize] ^ (c >> 8);
    }
    c ^ 0xFFFF_FFFF
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn crc32_matches_zlib() {
        assert_eq!(crc32(b"123456789"), 0xCBF4_3926);
    }

    #[test]
    fn refuses_garbage() {
        assert!(DataSet::from_compact("x", b"not a siren data file at all....").is_err());
    }
}
