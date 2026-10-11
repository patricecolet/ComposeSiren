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
