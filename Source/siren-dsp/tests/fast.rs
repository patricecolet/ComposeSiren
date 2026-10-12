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
