//! `sirene::Timing` away from 44.1 kHz: `Time` (the default) keeps what lasts
//! in seconds, `Plugin` counts in samples as the C++ does (`tests/oracle.rs`
//! and `tests/midi_in.rs` check it against the C++ at other rates). The two
//! are one at 44.1 kHz.

use siren_dsp::Library;
use siren_dsp::siren::SirenId;
use siren_dsp::sirene::{FrameControl, LabFrames, Timing, Trio, Voice};

fn lib() -> Library {
    Library::new(siren_dsp::default_resources())
}

#[derive(Clone, Copy)]
enum Ev {
    On(i32, i32),
    Off(i32),
    Cc(i32, i32),
}

/// The plugin's MIDI path, with the controllers that run on its timers:
/// vibrato with its attack, tremolo, portamento, attack and release.
const EVENTS: &[(f64, Ev)] = &[
    (0.0, Ev::Cc(1, 60)),
    (0.0, Ev::Cc(9, 50)),
    (0.0, Ev::Cc(11, 30)),
    (0.0, Ev::Cc(15, 30)),
    (0.0, Ev::Cc(92, 70)),
    (0.0, Ev::Cc(5, 60)),
    (0.0, Ev::Cc(73, 50)),
    (0.0, Ev::Cc(72, 50)),
    (0.0, Ev::On(72, 120)),
    (1.5, Ev::On(76, 120)),
    (3.0, Ev::Off(76)),
];
const SECONDS: f64 = 6.0;

fn render(v: &mut Voice, sr: f64) -> Vec<f32> {
    let at = |t: f64| (t * sr).round() as usize;
    let mut next = 0;
    (0..at(SECONDS))
        .map(|i| {
            while next < EVENTS.len() && at(EVENTS[next].0) <= i {
                match EVENTS[next].1 {
                    Ev::On(n, vel) => v.note_on(n, vel),
                    Ev::Off(n) => v.note_off(n),
                    Ev::Cc(cc, val) => v.control_change(cc, val),
                }
                next += 1;
            }
            v.process()
        })
        .collect()
}

/// the plugin's synthesis, or the locked trio over lab frames scaled with the shift
fn voice(lib: &Library, sr: f64, timing: Timing, trio: bool) -> Voice {
    let mut v = lib.voice(SirenId::S1, sr).unwrap();
    v.set_timing(timing);
    // no rotor spin-up from 0 (2 s of silence): the note sounds at once
    v.glide = false;
    if trio {
        v.siren.frames = FrameControl::Lab(LabFrames::full_loops(v.siren.data()));
        v.siren.set_trio(Some(Trio { scale_frames: true, lock: true, gains: siren_dsp::sirene::Gains::Linear, ..Trio::default() }));
    }
    v
}

/// RMS per 5 ms window, a level curve in time that does not depend on the rate
fn level(x: &[f32], sr: f64) -> Vec<f64> {
    let w = (0.005 * sr).round() as usize;
    x.chunks_exact(w).map(|c| (c.iter().map(|&s| s as f64 * s as f64).sum::<f64>() / w as f64).sqrt()).collect()
}

/// how far two level curves are apart, in dB (RMS of the dB difference where
/// either is within 40 dB of the peak)
fn level_distance(a: &[f64], b: &[f64]) -> f64 {
    let peak = a.iter().chain(b).fold(0.0f64, |m, &x| m.max(x));
    let db = |x: f64| 20.0 * (x.max(peak * 1e-3)).log10();
    let d: Vec<f64> = a.iter().zip(b).filter(|&(&x, &y)| x.max(y) > peak * 0.01).map(|(&x, &y)| db(x) - db(y)).collect();
    (d.iter().map(|x| x * x).sum::<f64>() / d.len() as f64).sqrt()
}

#[test]
fn time_and_plugin_are_one_at_44k() {
    let lib = lib();
    for trio in [false, true] {
        let a = render(&mut voice(&lib, 44100.0, Timing::Time, trio), 44100.0);
        let b = render(&mut voice(&lib, 44100.0, Timing::Plugin, trio), 44100.0);
        let first = a.iter().zip(&b).position(|(x, y)| x.to_bits() != y.to_bits());
        let peak = a.iter().fold(0.0f32, |m, x| m.max(x.abs()));
        eprintln!("trio {trio}: peak {peak}, first differing sample {first:?}");
        assert_eq!(first, None, "trio {trio}: Time and Plugin differ at 44.1 kHz");
        assert!(peak > 0.01);
    }
}

#[test]
fn time_keeps_the_timing_at_other_rates() {
    let lib = lib();
    for trio in [false, true] {
        let reference = level(&render(&mut voice(&lib, 44100.0, Timing::Time, trio), 44100.0), 44100.0);
        // (not under 44.1 kHz: what aliases changes the level, whatever the timing)
        for sr in [48000.0, 88200.0, 96000.0] {
            let time = level(&render(&mut voice(&lib, sr, Timing::Time, trio), sr), sr);
            let plugin = level(&render(&mut voice(&lib, sr, Timing::Plugin, trio), sr), sr);
            let (dt, dp) = (level_distance(&reference, &time), level_distance(&reference, &plugin));
            eprintln!("trio {trio} @{sr}: level curve vs 44.1 kHz, Time {dt:.2} dB, Plugin {dp:.2} dB");
            assert!(dt < 1.0, "trio {trio} @{sr}: Time is {dt:.2} dB away from 44.1 kHz");
            assert!(dp > dt + 0.25, "trio {trio} @{sr}: Plugin ({dp:.2} dB) should drift from 44.1 kHz more than Time ({dt:.2} dB)");
        }
    }
}

/// The data's frames last `fdur` samples at 44.1 kHz: with `Time` a frame
/// changes at the same moments at any rate, with `Plugin` 9 % more often at 48
/// kHz (and twice as often at 88.2 kHz).
#[test]
fn a_frame_lasts_the_same_in_seconds() {
    let lib = lib();
    // the moments, in seconds, when data note 60's frame changes over 2 s
    let changes = |sr: f64, timing: Timing| {
        let mut v = lib.voice(SirenId::S1, sr).unwrap();
        v.set_timing(timing);
        v.glide = false;
        v.siren.frames = FrameControl::Lab(LabFrames::full_loops(v.siren.data()));
        v.note_on(72, 120);
        let mut last = v.siren.frame_of(60);
        let mut at = Vec::new();
        for i in 0..(2.0 * sr) as usize {
            v.process();
            let f = v.siren.frame_of(60);
            if f != last {
                at.push(i as f64 / sr);
                last = f;
            }
        }
        at
    };
    let reference = changes(44100.0, Timing::Time);
    assert!(reference.len() > 20);
    for sr in [48000.0, 88200.0, 96000.0] {
        let time = changes(sr, Timing::Time);
        let plugin = changes(sr, Timing::Plugin);
        eprintln!("@{sr}: {} frame changes in 2 s at 44.1 kHz, Time {}, Plugin {}", reference.len(), time.len(), plugin.len());
        assert_eq!(time.len(), reference.len(), "@{sr}: Time changes frames {} times, 44.1 kHz {}", time.len(), reference.len());
        let drift = time.iter().zip(&reference).map(|(a, b)| (a - b).abs()).fold(0.0f64, f64::max);
        // within a sample at 44.1 kHz, plus the f32 rounding of the scaled lengths
        assert!(drift < 2.0 / 44100.0, "@{sr}: a frame change {drift} s away from 44.1 kHz's");
        assert!(plugin.len() as f64 > reference.len() as f64 * 1.05, "@{sr}: Plugin should change frames faster");
    }
}
