//! Two layouts of the same frames, to measure what the layout changes.
//!
//! - [`LegacyTables`]: the plugin's `float tab[80][1000][200]`, twice (amp,
//!   freq), 128 MB per voice whatever the data set holds.
//! - [`Bank`]: what a Rust DSP could hold instead. Built once per data set
//!   (off the audio thread) and shared by every voice that uses it:
//!   - only the frames that hold data (`max(fmax, last frame with data)`),
//!   - only the partial slots a note ever uses, rounded up to [`LANES`],
//!   - one frame = one contiguous, cache-line aligned span
//!     `[freq; stride][amp; stride]`, so a frame change reads one run of
//!     memory and every SIMD load is aligned.

use std::fs::File;
use std::io::{self, Read};
use std::path::Path;

use crate::data::{DataSet, MAX_PARTIEL, MAX_TAB, NOMBRE_DE_NOTE};

/// f32 lanes of a 512-bit vector: the padding of a frame's partial slots.
pub const LANES: usize = 16;

static ZEROS: [f32; MAX_PARTIEL] = [0.0; MAX_PARTIEL];

/// A frame's partials, wherever they live.
pub trait FrameSource {
    /// `(freq, amp)` of frame `f` of note `n`, both as long as the note's
    /// stride (zeros past the data)
    fn frame(&self, n: usize, f: usize) -> (&[f32], &[f32]);
    fn fdur(&self, n: usize) -> usize;
    fn fmax(&self, n: usize) -> usize;
}

impl FrameSource for DataSet {
    #[inline]
    fn frame(&self, n: usize, f: usize) -> (&[f32], &[f32]) {
        (&self.freq(n, f)[..], &self.amp(n, f)[..])
    }
    fn fdur(&self, n: usize) -> usize {
        self.dure[n][0] as usize
    }
    fn fmax(&self, n: usize) -> usize {
        self.dure[n][1] as usize
    }
}

impl<T: FrameSource + ?Sized> FrameSource for &T {
    #[inline]
    fn frame(&self, n: usize, f: usize) -> (&[f32], &[f32]) {
        (**self).frame(n, f)
    }
    fn fdur(&self, n: usize) -> usize {
        (**self).fdur(n)
    }
    fn fmax(&self, n: usize) -> usize {
        (**self).fmax(n)
    }
}

impl<T: FrameSource + ?Sized> FrameSource for std::sync::Arc<T> {
    #[inline]
    fn frame(&self, n: usize, f: usize) -> (&[f32], &[f32]) {
        (**self).frame(n, f)
    }
    fn fdur(&self, n: usize) -> usize {
        (**self).fdur(n)
    }
    fn fmax(&self, n: usize) -> usize {
        (**self).fmax(n)
    }
}

/// The plugin's tables, read the way `Sirene::readDataFromBinaryFile` does.
pub struct LegacyTables {
    pub amp: Vec<f32>,
    pub freq: Vec<f32>,
    pub dure: [[f32; 3]; NOMBRE_DE_NOTE],
}

impl LegacyTables {
    pub fn load(dir: &Path, name: &str) -> io::Result<LegacyTables> {
        let len = NOMBRE_DE_NOTE * MAX_TAB * MAX_PARTIEL;
        let read = |file: String, out: &mut [f32]| -> io::Result<()> {
            let mut bytes = Vec::new();
            File::open(dir.join(file))?.read_to_end(&mut bytes)?;
            for (d, s) in out.iter_mut().zip(bytes.chunks_exact(4)) {
                *d = f32::from_le_bytes([s[0], s[1], s[2], s[3]]);
            }
            Ok(())
        };
        let mut amp = vec![0.0f32; len];
        let mut freq = vec![0.0f32; len];
        let mut dure_flat = [0.0f32; NOMBRE_DE_NOTE * 3];
        read(format!("dataAmp{name}"), &mut amp)?;
        read(format!("dataFreq{name}"), &mut freq)?;
        read(format!("datadureTabs{name}"), &mut dure_flat)?;
        let mut dure = [[0.0; 3]; NOMBRE_DE_NOTE];
        for (d, s) in dure.iter_mut().zip(dure_flat.chunks_exact(3)) {
            d.copy_from_slice(s);
        }
        Ok(LegacyTables { amp, freq, dure })
    }

    pub fn bytes(&self) -> usize {
        (self.amp.len() + self.freq.len()) * 4
    }
}

impl FrameSource for LegacyTables {
    #[inline]
    fn frame(&self, n: usize, f: usize) -> (&[f32], &[f32]) {
        if n >= NOMBRE_DE_NOTE || f >= MAX_TAB {
            return (&ZEROS, &ZEROS);
        }
        let o = (n * MAX_TAB + f) * MAX_PARTIEL;
        (&self.freq[o..o + MAX_PARTIEL], &self.amp[o..o + MAX_PARTIEL])
    }
    fn fdur(&self, n: usize) -> usize {
        self.dure[n][0] as usize
    }
    fn fmax(&self, n: usize) -> usize {
        self.dure[n][1] as usize
    }
}

/// 64-byte aligned storage unit
#[derive(Clone, Copy)]
#[repr(C, align(64))]
pub struct Line(pub [f32; LANES]);

#[derive(Clone, Copy, Debug, Default)]
pub struct NoteBlock {
    /// in [`Line`]s from the start of the pool
    pub offset: usize,
    pub frames: usize,
    pub fmax: usize,
    pub fdur: usize,
    /// partial slots kept per frame, a multiple of [`LANES`] (0: no data)
    pub stride: usize,
    pub mean_freq: f32,
}

pub struct Bank {
    pub notes: Vec<NoteBlock>,
    pool: Vec<Line>,
}

impl Bank {
    pub fn build(data: &DataSet) -> Bank {
        let mut notes = Vec::with_capacity(NOMBRE_DE_NOTE);
        let mut lines = 0;
        for info in &data.info {
            let frames = info.frames;
            let stride = info.max_partials_all.div_ceil(LANES) * LANES;
            notes.push(NoteBlock {
                offset: lines,
                frames,
                fmax: info.fmax,
                fdur: info.fdur.max(0.0) as usize,
                stride,
                mean_freq: info.mean_freq,
            });
            lines += frames * 2 * stride / LANES;
        }
        let mut pool = vec![Line([0.0; LANES]); lines];
        {
            let flat = Self::flat_mut(&mut pool);
            for (n, b) in notes.iter().enumerate() {
                for f in 0..b.frames {
                    let o = b.offset * LANES + f * 2 * b.stride;
                    // a stride rounded up past 200 slots keeps zeros at the end
                    let k = b.stride.min(MAX_PARTIEL);
                    flat[o..o + k].copy_from_slice(&data.freq(n, f)[..k]);
                    flat[o + b.stride..o + b.stride + k].copy_from_slice(&data.amp(n, f)[..k]);
                }
            }
        }
        Bank { notes, pool }
    }

    fn flat_mut(pool: &mut [Line]) -> &mut [f32] {
        // SAFETY: Line is repr(C) [f32; LANES] with no padding (64 bytes)
        unsafe { std::slice::from_raw_parts_mut(pool.as_mut_ptr().cast::<f32>(), pool.len() * LANES) }
    }

    fn flat(&self) -> &[f32] {
        // SAFETY: as above
        unsafe { std::slice::from_raw_parts(self.pool.as_ptr().cast::<f32>(), self.pool.len() * LANES) }
    }

    pub fn bytes(&self) -> usize {
        self.pool.len() * std::mem::size_of::<Line>() + self.notes.len() * std::mem::size_of::<NoteBlock>()
    }

    pub fn stride(&self, n: usize) -> usize {
        self.notes.get(n).map_or(0, |b| b.stride)
    }
}

impl FrameSource for Bank {
    #[inline]
    fn frame(&self, n: usize, f: usize) -> (&[f32], &[f32]) {
        match self.notes.get(n) {
            Some(b) if f < b.frames => {
                let o = b.offset * LANES + f * 2 * b.stride;
                let flat = self.flat();
                (&flat[o..o + b.stride], &flat[o + b.stride..o + 2 * b.stride])
            }
            _ => (&ZEROS[..0], &ZEROS[..0]),
        }
    }
    fn fdur(&self, n: usize) -> usize {
        self.notes.get(n).map_or(0, |b| b.fdur)
    }
    fn fmax(&self, n: usize) -> usize {
        self.notes.get(n).map_or(0, |b| b.fmax)
    }
}
