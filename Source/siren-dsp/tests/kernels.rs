//! The study's kernels against the port, on a held note.


use std::path::PathBuf;

use siren_dsp::Library;
use siren_dsp::data::DataSet;
use siren_dsp::kernels::{Held, Kernel, PhasorPartials, PhasorTime, ScalarSin};
use siren_dsp::layout::{Bank, LegacyTables};
use siren_dsp::siren::SirenId;
use siren_dsp::sirene::{FrameControl, FrameSel, LabFrames, Timing};

fn resources() -> PathBuf {
    siren_dsp::default_resources()
}

/// the port: lab loop, 16-bit, glide off, at the note's pitch
fn port(lib: &Library, id: SirenId, h: &Held, samples: usize) -> Vec<f32> {
    let mut v = lib.voice(id, h.sample_rate).unwrap();
    // the kernels count frames and smoothing in samples
    v.set_timing(Timing::Plugin);
    v.glide = false;
    v.siren.set_16ou8bit(false);
    v.siren.change_qualite(h.partials as i32);
    let mut l = LabFrames::full_loops(v.siren.data());
    l.sel[h.note] = FrameSel { start: h.start, end: h.end, freeze: None };
    v.siren.frames = FrameControl::Lab(l);
    v.note_on(h.note as i32 + 12, 127);
    (0..samples).map(|_| v.process()).collect()
}

fn run(k: &mut dyn Kernel, samples: usize) -> Vec<f32> {
    let mut out = vec![0.0; samples];
    // odd block sizes: frame changes fall inside blocks and on their edges
    let mut t = 0;
    for (i, n) in [512usize, 1, 37, 128, 1000].iter().cycle().enumerate() {
        if t >= samples || i > 1_000_000 {
            break;
        }
        let n = (*n).min(samples - t);
        k.render(&mut out[t..t + n]);
        t += n;
    }
    out
}

fn snr_db(reference: &[f32], got: &[f32]) -> f64 {
    let (mut s, mut e) = (0.0f64, 0.0f64);
    for (r, g) in reference.iter().zip(got) {
        s += (*r as f64).powi(2);
        e += (*r as f64 - *g as f64).powi(2);
    }
    if e == 0.0 { f64::INFINITY } else { 10.0 * (s / e).log10() }
}

#[test]
fn kernels_against_the_port() {
    let lib = Library::new(resources());
    let cases = [
        (SirenId::S1, "S1", Held { note: 60, start: 0, end: 163, partials: 93, sample_rate: 44100.0 }),
        (SirenId::S1, "S1", Held { note: 36, start: 5, end: 40, partials: 30, sample_rate: 44100.0 }),
        (SirenId::S4, "S4", Held { note: 48, start: 0, end: 261, partials: 150, sample_rate: 48000.0 }),
        (SirenId::S7, "S7", Held { note: 72, start: 0, end: 118, partials: 21, sample_rate: 44100.0 }),
    ];
    for (id, name, h) in cases {
        let samples = 3 * h.sample_rate as usize;
        let want = port(&lib, id, &h, samples);
        let data: &DataSet = &lib.data(id.def()).unwrap();
        let legacy = LegacyTables::load(&resources(), name).unwrap();
        let bank = Bank::build(data);

        // the same arithmetic over three layouts: the same samples
        for (layout, got) in [
            ("DataSet", run(&mut ScalarSin::new(data, h), samples)),
            ("LegacyTables", run(&mut ScalarSin::new(&legacy, h), samples)),
            ("Bank", run(&mut ScalarSin::new(&bank, h), samples)),
        ] {
            let first = want.iter().zip(&got).position(|(a, b)| a.to_bits() != b.to_bits());
            assert_eq!(first, None, "{name} {h:?}: ScalarSin over {layout} differs from the port");
        }

        // the phasors: f32 rounding within a frame, over 3 s
        let pp = snr_db(&want, &run(&mut PhasorPartials::new(&bank, h), samples));
        let pt = snr_db(&want, &run(&mut PhasorTime::new(&bank, h), samples));
        eprintln!("{name} note {} q{} @{}: PhasorPartials {pp:.1} dB, PhasorTime {pt:.1} dB", h.note, h.partials, h.sample_rate);
        // re-anchored every frame, the f32 phasors stay over 90 dB from the f64 phase
        assert!(pp > 90.0 && pt > 90.0, "{name} {h:?}: phasor kernels too far from the port");
    }
}
