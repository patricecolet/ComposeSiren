//! The fast trio (`trio.fast`, the `*-fast` variants) against the trio it speeds up: its sine
//! within 1e-9 of libm's, and every siren's output within 1e-6 of the peak (f32 output; the
//! measured worst case is printed with `--nocapture`).

use siren_dsp::Library;
use siren_dsp::fastsin;
use siren_dsp::settings::{DspSettings, variant};
use siren_dsp::siren::SirenId;

#[test]
fn the_sine_is_libm_within_1e_9() {
    let mut worst = 0.0f64;
    let mut x = -2000.0f64;
    while x < 2000.0 {
        worst = worst.max((fastsin::sin(x) - x.sin()).abs());
        x += 0.000_731;
    }
    println!("fastsin: max |error| {worst:.2e} over [-2000, 2000]");
    assert!(worst < 1e-9, "{worst:e}");
    let p: Vec<f64> = (0..37).map(|i| i as f64 * 1.37).collect();
    let a: Vec<f64> = (0..37).map(|i| 1.0 / (1.0 + i as f64)).collect();
    let want: f64 = p.iter().zip(&a).map(|(p, a)| p.sin() * a).sum();
    assert!((fastsin::sin_dot(&p, &a) - want).abs() < 1e-9);
}

fn render(lib: &Library, id: SirenId, s: &DspSettings, sr: f64, note: i32, samples: usize) -> Vec<f32> {
    let mut v = lib.voice(id, sr).unwrap();
    s.apply(&mut v);
    v.note_on(note, 127);
    (0..samples).map(|_| v.process()).collect()
}

#[test]
fn fast_variants_sound_as_the_trio() {
    let lib = Library::new(siren_dsp::default_resources());
    let sirens = [
        (SirenId::S1, 72),
        (SirenId::S2, 70),
        (SirenId::S3, 66),
        (SirenId::S4, 60),
        (SirenId::S5, 55),
        (SirenId::S6, 50),
        (SirenId::S7, 88),
    ];
    for (slow, fast) in [("trio", "trio-fast"), ("trio-linear", "trio-linear-fast"), ("trio-nearest", "trio-nearest-fast")] {
        let (s, f) = (variant(slow).unwrap().dsp_settings().unwrap(), variant(fast).unwrap().dsp_settings().unwrap());
        let mut worst = 0.0f32;
        for (id, note) in sirens {
            for sr in [44100.0, 48000.0] {
                let n = 4 * sr as usize;
                let (a, b) = (render(&lib, id, &s, sr, note, n), render(&lib, id, &f, sr, note, n));
                let peak = a.iter().fold(0.0f32, |m, x| m.max(x.abs()));
                assert!(peak > 1e-3, "{slow} {id:?}: silent");
                let diff = a.iter().zip(&b).fold(0.0f32, |m, (x, y)| m.max((x - y).abs()));
                worst = worst.max(diff / peak);
                assert!(diff <= 1e-6 * peak, "{fast} {id:?} @{sr}: {diff:e} against a peak of {peak}");
            }
        }
        println!("{fast}: max |difference| / peak against {slow}: {worst:.2e}");
    }
}

/// The f32 phasor variants against the trio: the worst peak-relative difference and the SNR
/// (trio power over difference power) over 7 sirens at 44.1 and 48 kHz, printed; the bounds
/// are the measured worst with margin (see docs/dsp-performance.md).
#[test]
fn f32_variants_sound_close_to_the_trio() {
    let lib = Library::new(siren_dsp::default_resources());
    let sirens = [(SirenId::S1, 72), (SirenId::S2, 70), (SirenId::S3, 66), (SirenId::S4, 60), (SirenId::S5, 55), (SirenId::S6, 50), (SirenId::S7, 88)];
    for (slow, quick) in [("trio", "trio-f32"), ("trio-linear", "trio-linear-f32"), ("trio-nearest", "trio-nearest-f32")] {
        let (s, f) = (variant(slow).unwrap().dsp_settings().unwrap(), variant(quick).unwrap().dsp_settings().unwrap());
        let (mut worst, mut snr_min) = (0.0f64, f64::INFINITY);
        for (id, note) in sirens {
            for sr in [44100.0, 48000.0] {
                let n = 4 * sr as usize;
                let (a, b) = (render(&lib, id, &s, sr, note, n), render(&lib, id, &f, sr, note, n));
                let peak = a.iter().fold(0.0f32, |m, x| m.max(x.abs())) as f64;
                let diff = a.iter().zip(&b).fold(0.0f32, |m, (x, y)| m.max((x - y).abs())) as f64;
                let sig: f64 = a.iter().map(|x| (*x as f64).powi(2)).sum();
                let err: f64 = a.iter().zip(&b).map(|(x, y)| ((*x - *y) as f64).powi(2)).sum();
                let snr = 10.0 * (sig / err.max(1e-300)).log10();
                worst = worst.max(diff / peak);
                snr_min = snr_min.min(snr);
                assert!(diff <= F32_PEAK * peak, "{quick} {id:?} @{sr}: {:e} of the peak", diff / peak);
                assert!(snr >= F32_SNR_DB, "{quick} {id:?} @{sr}: SNR {snr:.1} dB");
            }
        }
        println!("{quick}: max |difference| / peak against {slow}: {worst:.2e}, min SNR {snr_min:.1} dB");
    }
}

const F32_PEAK: f64 = 3e-5; // measured 6.1e-6
const F32_SNR_DB: f64 = 100.0; // measured 116.3 dB

#[test]
fn harmonic_sines_within_5e_4() {
    let mut out = [0.0f32; siren_dsp::sirene::TRIO_HARMONICS];
    let mut worst = 0.0f64;
    for k in 0..2000 {
        let phi = k as f64 * 0.003_17;
        siren_dsp::sirene::harmonic_sines(phi, &mut out);
        for (h, s) in out.iter().enumerate().skip(1) {
            worst = worst.max((*s as f64 - (h as f64 * phi).sin()).abs());
        }
    }
    println!("harmonic_sines: max |error| {worst:.2e} up to h = {}", out.len() - 1);
    assert!(worst < 5e-4, "{worst:e}"); // measured 2.2e-4 at h up to 511
}

/// The C ABI's `sdsp_voice_apply_variant` and `sdsp_voice_set_trio_fast` play what the settings do.
#[test]
fn the_c_abi_sets_variants() {
    use siren_dsp::ffi::{sdsp_voice_apply_variant, sdsp_voice_set_trio_fast};
    let lib = Library::new(siren_dsp::default_resources());
    for id in ["trio-f32", "trio-fast", "frames", "rust-exact"] {
        let want = render(&lib, SirenId::S3, &variant(id).unwrap().dsp_settings().unwrap(), 48000.0, 66, 48000);
        let mut v = lib.voice(SirenId::S3, 48000.0).unwrap();
        let c = std::ffi::CString::new(id).unwrap();
        assert_eq!(unsafe { sdsp_voice_apply_variant(&mut v, c.as_ptr()) }, 1, "{id}");
        v.note_on(66, 127);
        let got: Vec<f32> = (0..48000).map(|_| v.process()).collect();
        assert!(got.iter().zip(&want).all(|(a, b)| a.to_bits() == b.to_bits()), "{id}");
    }
    let mut v = lib.voice(SirenId::S3, 48000.0).unwrap();
    let c = std::ffi::CString::new("cpp").unwrap();
    assert_eq!(unsafe { sdsp_voice_apply_variant(&mut v, c.as_ptr()) }, 0);
    // trio → trio-f32 by the switch
    variant("trio").unwrap().dsp_settings().unwrap().apply(&mut v);
    unsafe { sdsp_voice_set_trio_fast(&mut v, true, true) };
    v.note_on(66, 127);
    let got: Vec<f32> = (0..48000).map(|_| v.process()).collect();
    let want = render(&lib, SirenId::S3, &variant("trio-f32").unwrap().dsp_settings().unwrap(), 48000.0, 66, 48000);
    assert!(got.iter().zip(&want).all(|(a, b)| a.to_bits() == b.to_bits()));
}
