//! ComposeSiren's C++ DSP as the plugin runs it, for siren-dsp's golden tests: one voice
//! (`Sirene` + `MidiIn`), CC 7 at 127 as the host sends it, stepped like
//! `SirenVoiceUnit::process()`.

use std::ffi::{CString, c_char, c_int, c_long, c_void};
use std::path::Path;

unsafe extern "C" {
    fn cso_voice_new(resources: *const c_char, siren: *const c_char, sample_rate: f64) -> *mut c_void;
    fn cso_voice_free(v: *mut c_void);
    fn cso_voice_set_quality(v: *mut c_void, q: c_int);
    fn cso_voice_set_crossfade(v: *mut c_void, on: c_int);
    fn cso_voice_set_8bit(v: *mut c_void, on: c_int);
    fn cso_voice_note_on(v: *mut c_void, note: c_int, velocity: c_int);
    fn cso_voice_note_off(v: *mut c_void, note: c_int);
    fn cso_voice_control_change(v: *mut c_void, cc: c_int, value: c_int);
    fn cso_voice_pitch_wheel(v: *mut c_void, lsb: c_int, msb: c_int);
    fn cso_voice_pitch(v: *mut c_void) -> f32;
    fn cso_voice_render(v: *mut c_void, out: *mut f32, n: c_long);

    fn cso_mareverbe_new() -> *mut c_void;
    fn cso_mareverbe_free(m: *mut c_void);
    fn cso_mareverbe_mute(m: *mut c_void);
    fn cso_mareverbe_set_room_size(m: *mut c_void, v: f32);
    fn cso_mareverbe_get_room_size(m: *mut c_void) -> f32;
    fn cso_mareverbe_set_damp(m: *mut c_void, v: f32);
    fn cso_mareverbe_get_damp(m: *mut c_void) -> f32;
    fn cso_mareverbe_set_wet(m: *mut c_void, v: f32);
    fn cso_mareverbe_get_wet(m: *mut c_void) -> f32;
    fn cso_mareverbe_set_dry(m: *mut c_void, v: f32);
    fn cso_mareverbe_get_dry(m: *mut c_void) -> f32;
    fn cso_mareverbe_set_width(m: *mut c_void, v: f32);
    fn cso_mareverbe_get_width(m: *mut c_void) -> f32;
    fn cso_mareverbe_set_mode(m: *mut c_void, v: f32);
    fn cso_mareverbe_get_mode(m: *mut c_void) -> f32;
    fn cso_mareverbe_process_stereo(m: *mut c_void, in_l: *mut f32, in_r: *mut f32, out_l: *mut f32, out_r: *mut f32, n: c_long);

    fn cso_reverb_new() -> *mut c_void;
    fn cso_reverb_free(r: *mut c_void);
    fn cso_reverb_set_sample_rate(r: *mut c_void, sr: f64);
    fn cso_reverb_begin_process_block(r: *mut c_void);
    fn cso_reverb_enable(r: *mut c_void, e: c_int);
    fn cso_reverb_set_dry_wet(r: *mut c_void, v: f32);
    fn cso_reverb_set_damping(r: *mut c_void, v: f32);
    fn cso_reverb_set_room_size(r: *mut c_void, v: f32);
    fn cso_reverb_set_width(r: *mut c_void, v: f32);
    fn cso_reverb_set_low_cut(r: *mut c_void, v: f32);
    fn cso_reverb_set_high_cut(r: *mut c_void, v: f32);
    fn cso_reverb_process(r: *mut c_void, in_l: *mut f32, in_r: *mut f32, out_l: *mut f32, out_r: *mut f32, n: c_int);
}

/// One C++ voice. Defaults as the plugin builds it: quality 30, crossfade off, 8-bit on.
pub struct CppVoice(*mut c_void);

impl CppVoice {
    /// `resources`: the directory of the raw data files; `siren`: "S1" .. "S7".
    pub fn new(resources: &Path, siren: &str, sample_rate: f64) -> Option<CppVoice> {
        let r = CString::new(resources.to_str()?).ok()?;
        let s = CString::new(siren).ok()?;
        let v = unsafe { cso_voice_new(r.as_ptr(), s.as_ptr(), sample_rate) };
        (!v.is_null()).then_some(CppVoice(v))
    }
    pub fn set_quality(&mut self, q: i32) {
        unsafe { cso_voice_set_quality(self.0, q) }
    }
    pub fn set_crossfade(&mut self, on: bool) {
        unsafe { cso_voice_set_crossfade(self.0, on as c_int) }
    }
    /// `set16ou8Bit(true)` means 8-bit
    pub fn set_8bit(&mut self, on: bool) {
        unsafe { cso_voice_set_8bit(self.0, on as c_int) }
    }
    pub fn note_on(&mut self, note: i32, velocity: i32) {
        unsafe { cso_voice_note_on(self.0, note, velocity) }
    }
    pub fn note_off(&mut self, note: i32) {
        unsafe { cso_voice_note_off(self.0, note) }
    }
    pub fn control_change(&mut self, cc: i32, value: i32) {
        unsafe { cso_voice_control_change(self.0, cc, value) }
    }
    pub fn pitch_wheel(&mut self, lsb: i32, msb: i32) {
        unsafe { cso_voice_pitch_wheel(self.0, lsb, msb) }
    }
    pub fn pitch(&mut self) -> f32 {
        unsafe { cso_voice_pitch(self.0) }
    }
    pub fn render(&mut self, out: &mut [f32]) {
        unsafe { cso_voice_render(self.0, out.as_mut_ptr(), out.len() as c_long) }
    }
}

impl Drop for CppVoice {
    fn drop(&mut self) {
        unsafe { cso_voice_free(self.0) }
    }
}

/// C++ `mareverbe` instance (`Source/ComposeSirenCore/lib/dsp/mareverbe.cpp`).
pub struct CppMareverbe(*mut c_void);

impl Default for CppMareverbe {
    fn default() -> Self {
        Self::new()
    }
}

impl CppMareverbe {
    pub fn new() -> Self {
        let p = unsafe { cso_mareverbe_new() };
        assert!(!p.is_null(), "cso_mareverbe_new failed");
        Self(p)
    }
    pub fn mute(&mut self) {
        unsafe { cso_mareverbe_mute(self.0) }
    }
    pub fn set_room_size(&mut self, v: f32) {
        unsafe { cso_mareverbe_set_room_size(self.0, v) }
    }
    pub fn room_size(&self) -> f32 {
        unsafe { cso_mareverbe_get_room_size(self.0) }
    }
    pub fn set_damp(&mut self, v: f32) {
        unsafe { cso_mareverbe_set_damp(self.0, v) }
    }
    pub fn damp(&self) -> f32 {
        unsafe { cso_mareverbe_get_damp(self.0) }
    }
    pub fn set_wet(&mut self, v: f32) {
        unsafe { cso_mareverbe_set_wet(self.0, v) }
    }
    pub fn wet(&self) -> f32 {
        unsafe { cso_mareverbe_get_wet(self.0) }
    }
    pub fn set_dry(&mut self, v: f32) {
        unsafe { cso_mareverbe_set_dry(self.0, v) }
    }
    pub fn dry(&self) -> f32 {
        unsafe { cso_mareverbe_get_dry(self.0) }
    }
    pub fn set_width(&mut self, v: f32) {
        unsafe { cso_mareverbe_set_width(self.0, v) }
    }
    pub fn width(&self) -> f32 {
        unsafe { cso_mareverbe_get_width(self.0) }
    }
    pub fn set_mode(&mut self, v: f32) {
        unsafe { cso_mareverbe_set_mode(self.0, v) }
    }
    pub fn mode(&self) -> f32 {
        unsafe { cso_mareverbe_get_mode(self.0) }
    }
    pub fn process_stereo(&mut self, in_l: &[f32], in_r: &[f32], out_l: &mut [f32], out_r: &mut [f32]) {
        let n = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        let mut l_in = in_l[..n].to_vec();
        let mut r_in = in_r[..n].to_vec();
        unsafe {
            cso_mareverbe_process_stereo(
                self.0,
                l_in.as_mut_ptr(),
                r_in.as_mut_ptr(),
                out_l.as_mut_ptr(),
                out_r.as_mut_ptr(),
                n as c_long,
            )
        }
    }
    pub fn process_stereo_in_place(&mut self, buf_l: &mut [f32], buf_r: &mut [f32]) {
        let n = buf_l.len().min(buf_r.len());
        unsafe {
            cso_mareverbe_process_stereo(
                self.0,
                buf_l.as_mut_ptr(),
                buf_r.as_mut_ptr(),
                buf_l.as_mut_ptr(),
                buf_r.as_mut_ptr(),
                n as c_long,
            )
        }
    }
}

impl Drop for CppMareverbe {
    fn drop(&mut self) {
        unsafe { cso_mareverbe_free(self.0) }
    }
}

/// C++ `Reverb` instance (`Source/ComposeSirenCore/Reverb.cpp`).
pub struct CppReverb(*mut c_void);

impl Default for CppReverb {
    fn default() -> Self {
        Self::new()
    }
}

impl CppReverb {
    pub fn new() -> Self {
        let p = unsafe { cso_reverb_new() };
        assert!(!p.is_null(), "cso_reverb_new failed");
        Self(p)
    }
    pub fn set_sample_rate(&mut self, sr: f64) {
        unsafe { cso_reverb_set_sample_rate(self.0, sr) }
    }
    pub fn begin_process_block(&mut self) {
        unsafe { cso_reverb_begin_process_block(self.0) }
    }
    pub fn enable(&mut self, e: bool) {
        unsafe { cso_reverb_enable(self.0, e as c_int) }
    }
    pub fn set_dry_wet(&mut self, v: f32) {
        unsafe { cso_reverb_set_dry_wet(self.0, v) }
    }
    pub fn set_damping(&mut self, v: f32) {
        unsafe { cso_reverb_set_damping(self.0, v) }
    }
    pub fn set_room_size(&mut self, v: f32) {
        unsafe { cso_reverb_set_room_size(self.0, v) }
    }
    pub fn set_width(&mut self, v: f32) {
        unsafe { cso_reverb_set_width(self.0, v) }
    }
    pub fn set_low_cut(&mut self, v: f32) {
        unsafe { cso_reverb_set_low_cut(self.0, v) }
    }
    pub fn set_high_cut(&mut self, v: f32) {
        unsafe { cso_reverb_set_high_cut(self.0, v) }
    }
    pub fn process(&mut self, in_l: &[f32], in_r: &[f32], out_l: &mut [f32], out_r: &mut [f32]) {
        let n = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        let mut l_in = in_l[..n].to_vec();
        let mut r_in = in_r[..n].to_vec();
        unsafe {
            cso_reverb_process(
                self.0,
                l_in.as_mut_ptr(),
                r_in.as_mut_ptr(),
                out_l.as_mut_ptr(),
                out_r.as_mut_ptr(),
                n as c_int,
            )
        }
    }
    pub fn process_in_place(&mut self, buf_l: &mut [f32], buf_r: &mut [f32]) {
        let n = buf_l.len().min(buf_r.len());
        unsafe {
            cso_reverb_process(
                self.0,
                buf_l.as_mut_ptr(),
                buf_r.as_mut_ptr(),
                buf_l.as_mut_ptr(),
                buf_r.as_mut_ptr(),
                n as c_int,
            )
        }
    }
}

impl Drop for CppReverb {
    fn drop(&mut self) {
        unsafe { cso_reverb_free(self.0) }
    }
}
