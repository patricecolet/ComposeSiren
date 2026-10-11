//! The C ABI the plugin uses when built with `COMPOSESIREN_RUST_DSP`
//! (`include/siren_dsp_plugin.h`): one synthesis ([`Sirene`]) per siren voice, driven by the
//! plugin's own C++ MIDI handling exactly where it drives its C++ `Sirene`. The data of each
//! resources folder is loaded once per process and shared by every voice.
//!
//! Not real-time: `csr_siren_new` loads (or shares) the data, call it off the audio thread.
//! Real-time: every other call; none allocates or locks.

use std::collections::HashMap;
use std::ffi::c_char;
use std::path::PathBuf;
use std::sync::{Arc, Mutex, OnceLock};

use crate::Library;
use crate::ffi::{guard, str_arg};
use crate::settings::{Engine, dataset, variant};
use crate::siren::SirenId;
use crate::sirene::Sirene;

fn library(dir: PathBuf) -> Arc<Library> {
    static LIBS: OnceLock<Mutex<HashMap<PathBuf, Arc<Library>>>> = OnceLock::new();
    let mut libs = LIBS.get_or_init(Default::default).lock().unwrap();
    libs.entry(dir.clone()).or_insert_with(|| Arc::new(Library::new(dir))).clone()
}

/// The folder a data set is read from: `<resources>/<dataset>` when the build ships it there,
/// else `<resources>` itself (the one set the build ships).
fn dataset_dir(resources: &str, set: &str) -> PathBuf {
    let sub = PathBuf::from(resources).join(set);
    if sub.is_dir() { sub } else { PathBuf::from(resources) }
}

/// A new synthesis for `siren` ("S1".."S7") playing `variant` (an id of `VARIANTS`, a Rust
/// one) on `dataset` (an id of `DATASETS`). Null on error (see `sdsp_last_error`).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_new(
    resources: *const c_char,
    siren: *const c_char,
    variant_id: *const c_char,
    dataset_id: *const c_char,
) -> *mut Sirene {
    guard(std::ptr::null_mut(), || {
        let resources = unsafe { str_arg(resources) }?;
        let name = unsafe { str_arg(siren) }?;
        let id = SirenId::parse(name).ok_or_else(|| format!("unknown siren {name}"))?;
        let vid = unsafe { str_arg(variant_id) }?;
        let v = variant(vid).ok_or_else(|| format!("unknown DSP variant {vid}"))?;
        if v.engine != Engine::Rust {
            return Err(format!("{vid} is not a Rust variant"));
        }
        let did = unsafe { str_arg(dataset_id) }?;
        dataset(did).ok_or_else(|| format!("unknown data set {did}"))?;
        let settings = v.dsp_settings().map_err(|e| e.to_string())?;
        let lib = library(dataset_dir(resources, did));
        let def = id.def();
        let vi = lib.vector_interval(def).map_err(|e| e.to_string())?;
        let data = lib.data(def).map_err(|e| e.to_string())?;
        let mut s = Sirene::new(def, data, &vi);
        s.set_timing(settings.timing);
        settings.apply_synthesis(&mut s);
        Ok(Box::into_raw(Box::new(s)))
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_free(s: *mut Sirene) {
    if !s.is_null() {
        drop(unsafe { Box::from_raw(s) });
    }
}

/// `Sirene::setSampleRate`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_set_sample_rate(s: *mut Sirene, sample_rate: f64) {
    if let Some(s) = unsafe { s.as_mut() } {
        s.set_sample_rate(sample_rate);
    }
}

/// `Sirene::setVelocite`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_set_velocite(s: *mut Sirene, velo: i32) {
    if let Some(s) = unsafe { s.as_mut() } {
        s.set_velocite(velo);
    }
}

/// `Sirene::setnoteFromExt`
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_set_note_from_ext(s: *mut Sirene, note: i32) {
    if let Some(s) = unsafe { s.as_mut() } {
        s.set_note_from_ext(note);
    }
}

/// `Sirene::setnote`: the 1 kHz control tick
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_setnote(s: *mut Sirene) {
    if let Some(s) = unsafe { s.as_mut() } {
        s.setnote();
    }
}

/// `Sirene::calculwave`: the next sample
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_calculwave(s: *mut Sirene) -> f32 {
    unsafe { s.as_mut() }.map_or(0.0, |s| s.calculwave())
}

/// `Sirene::getCurrentPitch`, in cents
#[unsafe(no_mangle)]
pub unsafe extern "C" fn csr_siren_current_pitch(s: *const Sirene) -> f32 {
    unsafe { s.as_ref() }.map_or(0.0, |s| s.current_pitch())
}
