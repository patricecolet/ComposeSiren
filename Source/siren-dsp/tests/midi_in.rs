//! The port's whole MidiIn path (CCs, pitch wheel, vibrato, tremolo,
//! portamento, attack, release, reset) against the plugin's C++, sample by
//! sample: the same events go to the C++ voice (`siren-dsp-oracle`) and to the Rust one.

use siren_dsp::Library;
use siren_dsp::siren::SirenId;
use siren_dsp::sirene::Timing;

use siren_dsp_oracle::CppVoice;

#[derive(Clone, Copy)]
enum Ev {
    On(i32, i32),
    Off(i32),
    Cc(i32, i32),
    Pw(i32, i32),
}

struct Case {
    name: &'static str,
    siren: &'static str,
    seconds: f64,
    quality: i32,
    crossfade: bool,
    bit16: bool,
    events: &'static [(f64, Ev)],
}

/// the case at sample rate `sr`, the Rust voice at `Timing::Plugin` (the same
/// as the default `Timing::Time` at 44.1 kHz)
fn check(lib: &Library, c: &Case, sr: f64) {
    let samples = (c.seconds * sr) as usize;
    let at = |t: f64| (t * sr) as usize;
    let mut cpp = CppVoice::new(&siren_dsp::default_resources(), c.siren, sr).expect("the C++ voice");
    cpp.set_quality(c.quality);
    cpp.set_crossfade(c.crossfade);
    cpp.set_8bit(!c.bit16);
    let mut next = 0;
    let mut want = vec![0.0f32; samples];
    for (i, w) in want.iter_mut().enumerate() {
        while next < c.events.len() && at(c.events[next].0) <= i {
            match c.events[next].1 {
                Ev::On(n, vel) => cpp.note_on(n, vel),
                Ev::Off(n) => cpp.note_off(n),
                Ev::Cc(cc, val) => cpp.control_change(cc, val),
                Ev::Pw(l, m) => cpp.pitch_wheel(l, m),
            }
            next += 1;
        }
        cpp.render(std::slice::from_mut(w));
    }

    let mut v = lib.voice(SirenId::parse(c.siren).unwrap(), sr).unwrap();
    v.set_timing(Timing::Plugin);
    v.siren.change_qualite(c.quality);
    v.siren.set_crossfade(c.crossfade);
    v.siren.set_16ou8bit(!c.bit16);
    let mut next = 0;
    let got: Vec<f32> = (0..samples)
        .map(|i| {
            while next < c.events.len() && at(c.events[next].0) <= i {
                match c.events[next].1 {
                    Ev::On(n, vel) => v.note_on(n, vel),
                    Ev::Off(n) => v.note_off(n),
                    Ev::Cc(cc, val) => v.control_change(cc, val),
                    Ev::Pw(l, m) => v.pitch_wheel(l, m),
                }
                next += 1;
            }
            v.process()
        })
        .collect();

    let first = got.iter().zip(&want).position(|(a, b)| a.to_bits() != b.to_bits());
    let peak = want.iter().fold(0.0f32, |m, x| m.max(x.abs()));
    eprintln!("{} @{sr}: {} {}, peak {peak:.4}, first differing sample {first:?}", c.name, c.siren, samples);
    assert!(peak > 0.0, "{}: the oracle is silent", c.name);
    assert_eq!(first, None, "{}: the port differs from the C++", c.name);
}

#[test]
fn midi_in_matches_the_cpp() {
    let lib = Library::new(siren_dsp::default_resources());
    use Ev::*;
    let cases = [
        Case {
            name: "vibrato",
            siren: "S1",
            seconds: 6.0,
            quality: 30,
            crossfade: false,
            bit16: false,
            events: &[(0.0, Cc(1, 64)), (0.0, Cc(9, 40)), (0.0, On(72, 100))],
        },
        Case {
            name: "vibrato-attack",
            siren: "S3",
            seconds: 6.0,
            quality: 40,
            crossfade: false,
            bit16: false,
            events: &[(0.0, Cc(1, 90)), (0.0, Cc(9, 70)), (0.0, Cc(11, 20)), (0.0, On(60, 127)), (3.0, On(64, 127))],
        },
        Case {
            name: "tremolo-release",
            siren: "S4",
            seconds: 7.0,
            quality: 30,
            crossfade: false,
            bit16: true,
            events: &[(0.0, Cc(15, 50)), (0.0, Cc(92, 100)), (0.0, Cc(72, 40)), (0.0, On(65, 110)), (4.5, Off(65))],
        },
        Case {
            name: "attack-release",
            siren: "S5",
            seconds: 6.0,
            quality: 30,
            crossfade: false,
            bit16: false,
            events: &[(0.0, Cc(73, 60)), (0.0, Cc(72, 30)), (0.0, On(78, 100)), (3.0, Off(78)), (4.5, On(76, 90))],
        },
        Case {
            name: "portamento",
            siren: "S1",
            seconds: 8.0,
            quality: 30,
            crossfade: true,
            bit16: false,
            events: &[(0.0, Cc(5, 100)), (0.0, On(60, 127)), (4.0, On(72, 127))],
        },
        Case {
            name: "pitch-wheel",
            siren: "S7",
            seconds: 7.0,
            quality: 21,
            crossfade: false,
            bit16: false,
            events: &[
                (0.0, Cc(16, 2)),
                (0.0, On(84, 127)),
                (2.5, Pw(0, 96)),
                (4.0, Pw(127, 31)),
                (5.5, Pw(0, 64)),
            ],
        },
        Case {
            name: "volume-reset",
            siren: "S2",
            seconds: 7.0,
            quality: 30,
            crossfade: false,
            bit16: false,
            events: &[(0.0, On(72, 127)), (2.5, Cc(7, 64)), (4.0, Cc(121, 0)), (4.5, On(70, 100))],
        },
        Case {
            name: "everything-crossfade-16bit",
            siren: "S6",
            seconds: 8.0,
            quality: 60,
            crossfade: true,
            bit16: true,
            events: &[
                (0.0, Cc(1, 50)),
                (0.0, Cc(9, 30)),
                (0.0, Cc(11, 10)),
                (0.0, Cc(15, 20)),
                (0.0, Cc(92, 60)),
                (0.0, Cc(5, 40)),
                (0.0, Cc(16, 12)),
                (0.0, Cc(73, 90)),
                (0.0, Cc(72, 90)),
                (0.0, On(74, 127)),
                (3.0, Pw(0, 80)),
                (5.0, Off(74)),
                (6.0, On(70, 64)),
            ],
        },
    ];
    for c in &cases {
        check(&lib, c, 44100.0);
    }
    // the plugin's timers at other rates, as `Timing::Plugin` reproduces them
    for (name, sr) in [("everything-crossfade-16bit", 48000.0), ("portamento", 96000.0), ("tremolo-release", 22050.0)] {
        check(&lib, cases.iter().find(|c| c.name == name).unwrap(), sr);
    }
}
