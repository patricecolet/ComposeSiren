//! C ABI for `reverb-dsp` (`include/reverb_dsp.h`), usable from C, C++, F# or any host
//! that calls C, and used by `Source/ComposeSirenCore/Reverb.cpp` when built with
//! `COMPOSESIREN_RUST_DSP=ON`.

use std::panic::{AssertUnwindSafe, catch_unwind};

use crate::mareverbe::Mareverbe;
use crate::reverb::{Reverb, ReverbMode};

pub const ABI_VERSION: u32 = 1;

fn guard<T>(fallback: T, f: impl FnOnce() -> T) -> T {
    catch_unwind(AssertUnwindSafe(f)).unwrap_or(fallback)
}

#[unsafe(no_mangle)]
pub extern "C" fn csr_reverb_abi_version() -> u32 {
    ABI_VERSION
}

// --- Reverb ----------------------------------------------------------------------

#[unsafe(no_mangle)]
pub extern "C" fn csr_reverb_new() -> *mut Reverb {
    guard(std::ptr::null_mut(), || Box::into_raw(Box::new(Reverb::new())))
}

/// # Safety
/// `r` comes from `csr_reverb_new` and is not used afterwards.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_free(r: *mut Reverb) {
    if !r.is_null() {
        drop(unsafe { Box::from_raw(r) });
    }
}

/// Sets the operating mode (`0` = [`ReverbMode::Plugin`], `1` = [`ReverbMode::StereoTime`]).
///
/// # Safety
/// `r` is a live `Reverb` pointer (same for every `csr_reverb_*` below).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_mode(r: *mut Reverb, mode: i32) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_mode(if mode == 1 { ReverbMode::StereoTime } else { ReverbMode::Plugin });
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_sample_rate(r: *mut Reverb, sample_rate: f64) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_sample_rate(sample_rate);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_begin_process_block(r: *mut Reverb) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.begin_process_block();
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_enable(r: *mut Reverb, e: bool) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.enable(e);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_dry_wet(r: *mut Reverb, v: f32) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_dry_wet(v);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_damping(r: *mut Reverb, v: f32) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_damping(v);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_room_size(r: *mut Reverb, v: f32) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_room_size(v);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_width(r: *mut Reverb, v: f32) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_width(v);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_low_cut(r: *mut Reverb, v: f32) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_low_cut(v);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_set_high_cut(r: *mut Reverb, v: f32) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.set_high_cut(v);
    }
}

/// # Safety
/// see `csr_reverb_set_mode`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_mute(r: *mut Reverb) {
    if let Some(r) = unsafe { r.as_mut() } {
        r.mute();
    }
}

/// Processes `n` stereo samples. Supports both separate buffers (`in_l != out_l`, `in_r != out_r`)
/// and in-place buffers (`in_l == out_l`, `in_r == out_r`).
///
/// # Safety
/// `r` is a live `Reverb`; `in_l`, `in_r` point to `n` readable floats and `out_l`, `out_r`
/// point to `n` writable floats.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_reverb_process(
    r: *mut Reverb,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    n: usize,
) {
    let Some(r) = (unsafe { r.as_mut() }) else {
        return;
    };
    if in_l.is_null() || in_r.is_null() || out_l.is_null() || out_r.is_null() {
        return;
    }
    if in_l.cast_mut() == out_l && in_r.cast_mut() == out_r {
        if out_l == out_r {
            return;
        }
        let buf_l = unsafe { std::slice::from_raw_parts_mut(out_l, n) };
        let buf_r = unsafe { std::slice::from_raw_parts_mut(out_r, n) };
        r.process_in_place(buf_l, buf_r);
    } else {
        let in_l = unsafe { std::slice::from_raw_parts(in_l, n) };
        let in_r = unsafe { std::slice::from_raw_parts(in_r, n) };
        let out_l = unsafe { std::slice::from_raw_parts_mut(out_l, n) };
        let out_r = unsafe { std::slice::from_raw_parts_mut(out_r, n) };
        r.process(in_l, in_r, out_l, out_r);
    }
}

// --- Mareverbe -------------------------------------------------------------------

#[unsafe(no_mangle)]
pub extern "C" fn csr_mareverbe_new() -> *mut Mareverbe {
    guard(std::ptr::null_mut(), || Box::into_raw(Box::new(Mareverbe::new())))
}

/// # Safety
/// `m` comes from `csr_mareverbe_new` and is not used afterwards.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_free(m: *mut Mareverbe) {
    if !m.is_null() {
        drop(unsafe { Box::from_raw(m) });
    }
}

/// # Safety
/// `m` is a live `Mareverbe` pointer (same for every `csr_mareverbe_*` below).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_mute(m: *mut Mareverbe) {
    if let Some(m) = unsafe { m.as_mut() } {
        m.mute();
    }
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_set_room_size(m: *mut Mareverbe, value: f32) {
    if let Some(m) = unsafe { m.as_mut() } {
        m.set_room_size(value);
    }
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_get_room_size(m: *const Mareverbe) -> f32 {
    unsafe { m.as_ref() }.map_or(0.0, Mareverbe::room_size)
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_set_damp(m: *mut Mareverbe, value: f32) {
    if let Some(m) = unsafe { m.as_mut() } {
        m.set_damp(value);
    }
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_get_damp(m: *const Mareverbe) -> f32 {
    unsafe { m.as_ref() }.map_or(0.0, Mareverbe::damp)
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_set_wet(m: *mut Mareverbe, value: f32) {
    if let Some(m) = unsafe { m.as_mut() } {
        m.set_wet(value);
    }
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_get_wet(m: *const Mareverbe) -> f32 {
    unsafe { m.as_ref() }.map_or(0.0, Mareverbe::wet)
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_set_dry(m: *mut Mareverbe, value: f32) {
    if let Some(m) = unsafe { m.as_mut() } {
        m.set_dry(value);
    }
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_get_dry(m: *const Mareverbe) -> f32 {
    unsafe { m.as_ref() }.map_or(0.0, Mareverbe::dry)
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_set_width(m: *mut Mareverbe, value: f32) {
    if let Some(m) = unsafe { m.as_mut() } {
        m.set_width(value);
    }
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_get_width(m: *const Mareverbe) -> f32 {
    unsafe { m.as_ref() }.map_or(0.0, Mareverbe::width)
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_set_mode(m: *mut Mareverbe, value: f32) {
    if let Some(m) = unsafe { m.as_mut() } {
        m.set_mode(value);
    }
}

/// # Safety
/// see `csr_mareverbe_mute`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_get_mode(m: *const Mareverbe) -> f32 {
    unsafe { m.as_ref() }.map_or(0.0, Mareverbe::mode)
}

/// # Safety
/// `m` is a live `Mareverbe`; `in_l`, `in_r` point to `n` readable floats and `out_l`, `out_r`
/// point to `n` writable floats.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_mareverbe_process_stereo(
    m: *mut Mareverbe,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    n: usize,
) {
    let Some(m) = (unsafe { m.as_mut() }) else {
        return;
    };
    if in_l.is_null() || in_r.is_null() || out_l.is_null() || out_r.is_null() {
        return;
    }
    if in_l.cast_mut() == out_l && in_r.cast_mut() == out_r {
        if out_l == out_r {
            return;
        }
        let buf_l = unsafe { std::slice::from_raw_parts_mut(out_l, n) };
        let buf_r = unsafe { std::slice::from_raw_parts_mut(out_r, n) };
        m.process_stereo_in_place(buf_l, buf_r);
    } else {
        let in_l = unsafe { std::slice::from_raw_parts(in_l, n) };
        let in_r = unsafe { std::slice::from_raw_parts(in_r, n) };
        let out_l = unsafe { std::slice::from_raw_parts_mut(out_l, n) };
        let out_r = unsafe { std::slice::from_raw_parts_mut(out_r, n) };
        m.process_stereo(in_l, in_r, out_l, out_r);
    }
}
