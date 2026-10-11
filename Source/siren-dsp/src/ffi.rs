//! C ABI, for C, C++, F# or anything else that can call C.
//!
//! Handles are opaque pointers, freed by their `_free` function. No call
//! unwinds across the boundary: a panic or an error returns null or 0, and
//! `sdsp_last_error` tells why.
//!
//! - `sdsp_library_*`: a `Resources/` directory, data sets loaded on first use
//! - `sdsp_voice_*`: the port of the plugin's voice (`SirenVoiceUnit`), sample
//!   for sample, with the lab's frame control and probes
//! - `sdsp_kernel_*`: the layout study's kernels on a held note
//! - `sdsp_render`: one note, start to end

use std::cell::RefCell;
use std::ffi::{CStr, c_char};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::Arc;

use crate::kernels::{Held, Kernel, PhasorPartials, PhasorTime, ScalarSin};
use crate::layout::{Bank, LegacyTables};
use crate::siren::SirenId;
use crate::sirene::{Edge, FrameControl, FrameSel, Gains, Kernel as TrioKernel, LabFrames, Timing, Trio, Voice};
use crate::{Frames, Library, RenderSpec};

pub const ABI_VERSION: u32 = 1;

thread_local! {
    static LAST_ERROR: RefCell<String> = const { RefCell::new(String::new()) };
}

pub(crate) fn set_error(e: impl Into<String>) {
    LAST_ERROR.with(|l| *l.borrow_mut() = e.into());
}

pub(crate) fn guard<T>(fallback: T, f: impl FnOnce() -> Result<T, String>) -> T {
    match catch_unwind(AssertUnwindSafe(f)) {
        Ok(Ok(v)) => v,
        Ok(Err(e)) => {
            set_error(e);
            fallback
        }
        Err(p) => {
            let msg = p
                .downcast_ref::<&str>()
                .map(|s| s.to_string())
                .or_else(|| p.downcast_ref::<String>().cloned())
                .unwrap_or_else(|| "panic".into());
            set_error(format!("panic: {msg}"));
            fallback
        }
    }
}

pub(crate) unsafe fn str_arg<'a>(p: *const c_char) -> Result<&'a str, String> {
    if p.is_null() {
        return Err("null string".into());
    }
    // SAFETY: the caller passes a NUL-terminated string
    unsafe { CStr::from_ptr(p) }.to_str().map_err(|e| e.to_string())
}

unsafe fn siren_arg(p: *const c_char) -> Result<SirenId, String> {
    let s = unsafe { str_arg(p) }?;
    SirenId::parse(s).ok_or_else(|| format!("unknown siren {s}"))
}

/// ABI version: bumped when a signature changes
#[unsafe(no_mangle)]
pub extern "C" fn sdsp_abi_version() -> u32 {
    ABI_VERSION
}

/// Copies the calling thread's last error into `buf` (NUL-terminated,
/// truncated to `len`). Returns the error's full length in bytes.
///
/// # Safety
/// `buf` is null or points to `len` writable bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_last_error(buf: *mut u8, len: usize) -> usize {
    LAST_ERROR.with(|l| {
        let e = l.borrow();
        if !buf.is_null() && len > 0 {
            let n = e.len().min(len - 1);
            // SAFETY: buf has len bytes
            unsafe {
                std::ptr::copy_nonoverlapping(e.as_ptr(), buf, n);
                *buf.add(n) = 0;
            }
        }
        e.len()
    })
}

// --- library -------------------------------------------------------------------

/// # Safety
/// `resources` is a NUL-terminated path, or null for the default
/// (`$COMPOSESIREN_RESOURCES`, or this repository's `Resources/`).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_library_new(resources: *const c_char) -> *mut Library {
    guard(std::ptr::null_mut(), || {
        let dir = if resources.is_null() { crate::default_resources() } else { unsafe { str_arg(resources) }?.into() };
        let lib = Library::new(dir.clone());
        // fail now on a wrong path rather than at the first voice
        lib.data(SirenId::S1.def()).map_err(|e| format!("{}: {e}", dir.display()))?;
        Ok(Box::into_raw(Box::new(lib)))
    })
}

/// # Safety
/// `lib` comes from `sdsp_library_new` and is not used afterwards. Voices and
/// kernels keep their data alive on their own.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_library_free(lib: *mut Library) {
    if !lib.is_null() {
        drop(unsafe { Box::from_raw(lib) });
    }
}

// --- voice ------------------------------------------------------------------------

/// # Safety
/// `lib` is a live library, `siren` a NUL-terminated id ("S1".."S7").
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_new(lib: *const Library, siren: *const c_char, sample_rate: f64) -> *mut Voice {
    guard(std::ptr::null_mut(), || {
        let lib = unsafe { lib.as_ref() }.ok_or("null library")?;
        let id = unsafe { siren_arg(siren) }?;
        let v = lib.voice(id, sample_rate).map_err(|e| e.to_string())?;
        Ok(Box::into_raw(Box::new(v)))
    })
}

/// # Safety
/// `v` comes from `sdsp_voice_new` and is not used afterwards.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_free(v: *mut Voice) {
    if !v.is_null() {
        drop(unsafe { Box::from_raw(v) });
    }
}

macro_rules! with_voice {
    ($v:expr, $body:expr) => {
        guard((), || {
            let v = unsafe { $v.as_mut() }.ok_or("null voice")?;
            #[allow(clippy::redundant_closure_call)]
            ($body)(v);
            Ok(())
        })
    };
}

/// partial slots summed (`changeQualite`, 30 in the plugin)
///
/// # Safety
/// `v` is a live voice (same for every `sdsp_voice_*` below).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_partials(v: *mut Voice, partials: i32) {
    with_voice!(v, |v: &mut Voice| v.siren.change_qualite(partials))
}

/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_crossfade(v: *mut Voice, on: bool) {
    with_voice!(v, |v: &mut Voice| v.siren.set_crossfade(on))
}

/// 8-bit mode: amplitudes and sines every other sample (the plugin's default)
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_8bit(v: *mut Voice, on: bool) {
    with_voice!(v, |v: &mut Voice| v.siren.set_16ou8bit(on))
}

/// the rotor's inertia (on in the plugin); off, a note jumps to its pitch
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_glide(v: *mut Voice, on: bool) {
    with_voice!(v, |v: &mut Voice| v.glide = on)
}

/// amplitude smoothing coefficient (0.001 in the plugin)
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_smoothing(v: *mut Voice, a: f64) {
    with_voice!(v, |v: &mut Voice| v.siren.set_smoothing(if a == 0.001 { (0.001, 0.999) } else { (a, 1.0 - a) }))
}

/// How the voice counts time away from 44.1 kHz (`sirene::Timing`): 0 time
/// (the default: timers, frames and ramps last what they last at 44.1 kHz),
/// 1 plugin (counted in samples, as the C++ does)
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_timing(v: *mut Voice, timing: i32) {
    with_voice!(v, |v: &mut Voice| v.set_timing(if timing == 1 { Timing::Plugin } else { Timing::Time }))
}

/// `MidiIn::handleControlChange`: CC 7 volume, 1/9/11 vibrato, 15/92 tremolo,
/// 5 portamento, 16 pitch-bend range, 72/73 release/attack, 121 reset
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_control_change(v: *mut Voice, cc: i32, value: i32) {
    with_voice!(v, |v: &mut Voice| v.control_change(cc, value))
}

/// `MidiIn::handlePitchWheel`: 14 bits (8192 = centre), range set by CC 16
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_pitch_wheel(v: *mut Voice, lsb: i32, msb: i32) {
    with_voice!(v, |v: &mut Voice| v.pitch_wheel(lsb, msb))
}

/// The three-note crossfade (`sirene::Trio`). `kernel`: 0 off (the plugin's
/// synthesis), 1 two notes linearly, 2 three notes (quadratic B-spline), 3 the
/// nearest note alone; `gains`: 0 linear, 1 equal-power; `edge`: 0 renormalise,
/// 1 centre, 2 reflect; `scale_frames`: lab frames follow the pitch shift;
/// `lock`: harmonic phase lock, the target fundamental being the pitch times
/// 2^`octave`. Keeps the tuning set by `sdsp_voice_set_tuning` and the sparse
/// settings of `sdsp_voice_set_sparse`.
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_trio(
    v: *mut Voice,
    kernel: i32,
    gains: i32,
    edge: i32,
    scale_frames: bool,
    lock: bool,
    octave: i32,
) {
    with_voice!(v, |v: &mut Voice| {
        let kept = v.siren.trio().cloned().unwrap_or_default();
        let trio = match kernel {
            1..=3 => Some(Trio {
                kernel: match kernel {
                    1 => TrioKernel::Linear2,
                    3 => TrioKernel::Nearest,
                    _ => TrioKernel::BSpline3,
                },
                gains: if gains == 0 { Gains::Linear } else { Gains::Power },
                edge: match edge {
                    1 => Edge::Centre,
                    2 => Edge::Reflect,
                    _ => Edge::Renormalize,
                },
                scale_frames,
                tuning: kept.tuning,
                lock,
                octave,
                spacing: kept.spacing,
                origin: kept.origin,
                envelope: kept.envelope,
                harmonic_envelope: kept.harmonic_envelope,
            }),
            _ => None,
        };
        v.siren.set_trio(trio)
    })
}

/// Sparse data for the trio (`sirene::Trio::spacing`, `origin`, `envelope`):
/// only the data notes `origin + j · spacing` play, the kernel over them, and
/// each partial's level corrected by its note's spectral envelope to the power
/// `envelope` (0 to 1). Spacing 1 and envelope 0: every note, as before. Only
/// with the trio on.
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_sparse(v: *mut Voice, spacing: i32, origin: i32, envelope: f64) {
    with_voice!(v, |v: &mut Voice| {
        if let Some(t) = v.siren.trio() {
            let t = Trio { spacing: spacing.max(1), origin, envelope: envelope.clamp(0.0, 1.0), ..t.clone() };
            v.siren.set_trio(Some(t))
        }
    })
}

/// With the phase lock, read the envelope correction at the harmonic each
/// partial sounds on rather than at its slot's frequency
/// (`sirene::Trio::harmonic_envelope`). Only with the trio on.
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_harmonic_envelope(v: *mut Voice, on: bool) {
    with_voice!(v, |v: &mut Voice| {
        if let Some(t) = v.siren.trio() {
            let t = Trio { harmonic_envelope: on, ..t.clone() };
            v.siren.set_trio(Some(t))
        }
    })
}

/// Per data note, the cents its partials sit above its nominal pitch, for the
/// trio's pitch shift (`n` = 0 clears it). Only with the trio on.
///
/// # Safety
/// see `sdsp_voice_set_partials`; `cents` points to `n` floats (or `n` = 0)
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_tuning(v: *mut Voice, cents: *const f32, n: usize) {
    // read unaligned: a caller's allocator (WebAssembly's, say) may only promise bytes
    let tuning: Vec<f32> =
        if n == 0 || cents.is_null() { Vec::new() } else { (0..n).map(|k| unsafe { cents.add(k).read_unaligned() }).collect() };
    with_voice!(v, |v: &mut Voice| {
        if let Some(t) = v.siren.trio() {
            let t = Trio { tuning, ..t.clone() };
            v.siren.set_trio(Some(t))
        }
    })
}

/// The trio's three notes and gains, lowest first (notes -1 when off).
///
/// # Safety
/// see `sdsp_voice_set_partials`; `notes` and `gains` point to 3 values each
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_trio_state(v: *const Voice, notes: *mut i32, gains: *mut f64) {
    if v.is_null() || notes.is_null() || gains.is_null() {
        return;
    }
    let st = unsafe { &*v }.siren.trio_state();
    for (k, (n, g)) in st.iter().enumerate() {
        unsafe {
            notes.add(k).write_unaligned(*n);
            gains.add(k).write_unaligned(*g);
        }
    }
}

/// the plugin's frames: counters reset every millisecond (frame 0)
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_frames_legacy(v: *mut Voice) {
    with_voice!(v, |v: &mut Voice| v.siren.frames = FrameControl::Legacy)
}

/// the lab's frames: every note loops on `[0, fmax)` until told otherwise
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_frames_lab(v: *mut Voice, speed: f32, interpolate: bool) {
    with_voice!(v, |v: &mut Voice| {
        let mut l = LabFrames::full_loops(v.siren.data());
        l.speed = speed;
        l.interpolate = interpolate;
        v.siren.frames = FrameControl::Lab(l);
    })
}

/// Lab frames of one note: loop `[start, end)`, or hold `freeze` (>= 0).
/// Switches to lab frames if the voice was on the plugin's.
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_set_selection(v: *mut Voice, note: i32, start: i32, end: i32, freeze: i32) {
    with_voice!(v, |v: &mut Voice| {
        if matches!(v.siren.frames, FrameControl::Legacy) {
            v.siren.frames = FrameControl::Lab(LabFrames::full_loops(v.siren.data()));
        }
        if let FrameControl::Lab(l) = &mut v.siren.frames
            && let Some(s) = l.sel.get_mut(note.max(0) as usize) {
                *s = FrameSel {
                    start: start.max(0) as usize,
                    end: end.max(0) as usize,
                    freeze: (freeze >= 0).then_some(freeze as usize),
                };
            }
    })
}

/// a MIDI note as the plugin receives it (data note + 12)
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_note_on(v: *mut Voice, midi_note: i32, velocity: i32) {
    with_voice!(v, |v: &mut Voice| v.note_on(midi_note, velocity))
}

/// the plugin's note off: the clapet closes, the rotor keeps turning
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_note_off(v: *mut Voice, midi_note: i32) {
    with_voice!(v, |v: &mut Voice| v.note_off(midi_note))
}

/// fade to silence
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_silence(v: *mut Voice) {
    with_voice!(v, |v: &mut Voice| v.siren.silence())
}

/// go to a pitch in midicents at once (no rotor inertia), e.g. between notes
///
/// # Safety
/// see `sdsp_voice_set_partials`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_jump_to(v: *mut Voice, midicent: i32) {
    with_voice!(v, |v: &mut Voice| v.siren.jump_to(midicent))
}

/// # Safety
/// see `sdsp_voice_set_partials`; `out` points to `n` writable floats.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_render(v: *mut Voice, out: *mut f32, n: usize) {
    with_voice!(v, |v: &mut Voice| {
        if out.is_null() {
            return;
        }
        // SAFETY: out has n floats
        let out = unsafe { std::slice::from_raw_parts_mut(out, n) };
        for o in out {
            *o = v.process();
        }
    })
}

/// What the voice is doing, for instrumentation.
#[repr(C)]
#[derive(Default)]
pub struct VoiceProbe {
    /// `noteEncour`, in midicents
    pub pitch: f32,
    /// `midiCentVoulue`, the midicent `setMidicent` settled on
    pub midicent: i32,
    pub note_inf: i32,
    pub frame_inf: i32,
    pub frame_sup: i32,
}

/// # Safety
/// see `sdsp_voice_set_partials`; `out` points to a writable `VoiceProbe`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_voice_probe(v: *const Voice, out: *mut VoiceProbe) {
    guard((), || {
        let v = unsafe { v.as_ref() }.ok_or("null voice")?;
        let out = unsafe { out.as_mut() }.ok_or("null probe")?;
        let n = v.siren.note_inf();
        *out = VoiceProbe {
            pitch: v.siren.current_pitch(),
            midicent: v.siren.midicent(),
            note_inf: n as i32,
            frame_inf: v.siren.frame_of(n) as i32,
            frame_sup: v.siren.frame_of(n + 1) as i32,
        };
        Ok(())
    })
}

// --- kernels ---------------------------------------------------------------------------

/// `ScalarSin` over the compact per-note frames (the port's `DataSet`)
pub const KERNEL_SCALAR_DATASET: i32 = 0;
/// `ScalarSin` over the aligned, trimmed `Bank`
pub const KERNEL_SCALAR_BANK: i32 = 1;
/// `ScalarSin` over the plugin's 128 MB tables, read from disk for this kernel
pub const KERNEL_SCALAR_LEGACY: i32 = 2;
pub const KERNEL_PHASOR_PARTIALS: i32 = 3;
pub const KERNEL_PHASOR_TIME: i32 = 4;

pub struct KernelBox(Box<dyn Kernel + Send>);

/// A kernel on note `note`, looping frames `[start, end)` with `partials`
/// slots. Kinds: the `KERNEL_*` constants.
///
/// # Safety
/// `lib` is a live library, `siren` a NUL-terminated id.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_kernel_new(
    lib: *const Library,
    siren: *const c_char,
    kind: i32,
    note: i32,
    start: i32,
    end: i32,
    partials: i32,
    sample_rate: f64,
) -> *mut KernelBox {
    guard(std::ptr::null_mut(), || {
        let lib = unsafe { lib.as_ref() }.ok_or("null library")?;
        let id = unsafe { siren_arg(siren) }?;
        let def = id.def();
        let h = Held {
            note: note.clamp(0, 79) as usize,
            start: start.max(0) as usize,
            end: end.max(start + 1) as usize,
            partials: partials.clamp(0, 200) as usize,
            sample_rate,
        };
        let data = lib.data(def).map_err(|e| e.to_string())?;
        let k: Box<dyn Kernel + Send> = match kind {
            KERNEL_SCALAR_DATASET => Box::new(ScalarSin::new(data, h)),
            KERNEL_SCALAR_BANK => Box::new(ScalarSin::new(Arc::new(Bank::build(&data)), h)),
            KERNEL_SCALAR_LEGACY => {
                let dir = lib.dir().ok_or("this library has no Resources directory, so no legacy tables")?;
                Box::new(ScalarSin::new(LegacyTables::load(dir, def.data).map_err(|e| e.to_string())?, h))
            }
            KERNEL_PHASOR_PARTIALS => Box::new(PhasorPartials::new(Arc::new(Bank::build(&data)), h)),
            KERNEL_PHASOR_TIME => Box::new(PhasorTime::new(Arc::new(Bank::build(&data)), h)),
            other => return Err(format!("unknown kernel kind {other}")),
        };
        Ok(Box::into_raw(Box::new(KernelBox(k))))
    })
}

/// # Safety
/// `k` is a live kernel; `out` points to `n` writable floats.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_kernel_render(k: *mut KernelBox, out: *mut f32, n: usize) {
    guard((), || {
        let k = unsafe { k.as_mut() }.ok_or("null kernel")?;
        if !out.is_null() {
            k.0.render(unsafe { std::slice::from_raw_parts_mut(out, n) });
        }
        Ok(())
    })
}

/// # Safety
/// `k` comes from `sdsp_kernel_new` and is not used afterwards.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_kernel_free(k: *mut KernelBox) {
    if !k.is_null() {
        drop(unsafe { Box::from_raw(k) });
    }
}

// --- one-shot render --------------------------------------------------------------------

/// `RenderSpec` for C: frames 0 = plugin (legacy), 1 = loop, 2 = freeze.
#[repr(C)]
pub struct RenderSpecC {
    pub note: i32,
    pub cents: i32,
    pub velocity: i32,
    pub seconds: f64,
    pub release_seconds: f64,
    pub sample_rate: f64,
    pub partials: i32,
    pub crossfade: bool,
    pub bit16: bool,
    pub glide: bool,
    pub frames: i32,
    pub start: i32,
    pub end: i32,
    pub freeze: i32,
    pub speed: f32,
    pub interpolate: bool,
    pub smoothing: f64,
}

/// Renders one note into `out` (capacity `cap` floats). Returns the number
/// of samples of the render, which may exceed `cap` (then only `cap` are
/// written): call with a null `out` to size the buffer. 0 on error.
///
/// # Safety
/// `lib` is a live library, `siren` a NUL-terminated id, `spec` readable,
/// `out` null or `cap` writable floats.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sdsp_render(
    lib: *const Library,
    siren: *const c_char,
    spec: *const RenderSpecC,
    out: *mut f32,
    cap: usize,
) -> usize {
    guard(0, || {
        let lib = unsafe { lib.as_ref() }.ok_or("null library")?;
        let c = unsafe { spec.as_ref() }.ok_or("null spec")?;
        let s = RenderSpec {
            siren: unsafe { siren_arg(siren) }?,
            note: c.note.clamp(0, 79) as usize,
            cents: c.cents.clamp(0, 99),
            velocity: c.velocity.clamp(0, 127),
            seconds: c.seconds.clamp(0.0, 600.0),
            release_seconds: c.release_seconds.clamp(0.0, 60.0),
            sample_rate: c.sample_rate.clamp(8000.0, 384000.0),
            partials: c.partials.clamp(0, 200),
            crossfade: c.crossfade,
            bit16: c.bit16,
            glide: c.glide,
            frames: match c.frames {
                0 => Frames::Legacy,
                1 => Frames::Loop { start: c.start.max(0) as usize, end: c.end.max(0) as usize },
                2 => Frames::Freeze(c.freeze.max(0) as usize),
                other => return Err(format!("bad frames {other}")),
            },
            speed: c.speed,
            interpolate: c.interpolate,
            smoothing: c.smoothing,
        };
        let total = ((s.seconds * s.sample_rate) as usize) + ((s.release_seconds * s.sample_rate) as usize);
        if out.is_null() {
            return Ok(total);
        }
        let samples = crate::render(lib, &s).map_err(|e| e.to_string())?;
        let n = samples.len().min(cap);
        unsafe { std::ptr::copy_nonoverlapping(samples.as_ptr(), out, n) };
        Ok(samples.len())
    })
}
