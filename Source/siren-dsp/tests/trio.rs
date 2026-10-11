//! The three-note crossfade (`sirene::Trio`): its weights, its edges, and that
//! it plays what it should (one bank = the plugin's note; no click when the
//! nearest note changes; frames per bank); on sparse data (anchors, envelope
//! correction).

use siren_dsp::Library;
use siren_dsp::siren::SirenId;
use siren_dsp::sirene::{Edge, Envelope, FrameControl, Gains, Kernel, LabFrames, Trio, Voice, edge_weights};

fn lib() -> Library {
    Library::new(siren_dsp::default_resources())
}

fn voice(lib: &Library, siren: &str, trio: Option<Trio>) -> Voice {
    let mut v = lib.voice(SirenId::parse(siren).unwrap(), 44100.0).unwrap();
    v.glide = false;
    v.siren.set_16ou8bit(false);
    v.siren.set_trio(trio);
    v
}

fn snr_db(reference: &[f32], other: &[f32]) -> f64 {
    let (s, e) = reference.iter().zip(other).fold((0.0, 0.0), |(s, e), (&r, &o)| {
        (s + r as f64 * r as f64, e + (r as f64 - o as f64).powi(2))
    });
    if e == 0.0 { f64::INFINITY } else { 10.0 * (s / e).log10() }
}

#[test]
fn every_edge_keeps_the_total() {
    for edge in [Edge::Renormalize, Edge::Centre, Edge::Reflect] {
        for mask in 1..8u8 {
            let has = [mask & 1 != 0, mask & 2 != 0, mask & 4 != 0];
            for k in 0..100 {
                let d = -0.5 + k as f64 / 100.0;
                let mut w = [(0.5 - d) * (0.5 - d) / 2.0, 0.75 - d * d, (0.5 + d) * (0.5 + d) / 2.0];
                edge_weights(edge, &mut w, has);
                let sum: f64 = w.iter().sum();
                assert!((sum - 1.0).abs() < 1e-12, "{edge:?} {has:?} d={d}: sum {sum}");
                for j in 0..3 {
                    assert!(has[j] || w[j] == 0.0, "{edge:?} {has:?}: a missing note kept weight");
                    assert!(w[j] >= 0.0);
                }
            }
        }
    }
}

#[test]
fn gains_sum_to_one_and_move_smoothly() {
    let lib = lib();
    for gains in [Gains::Linear, Gains::Power] {
        let v = voice(&lib, "S1", Some(Trio { gains, ..Trio::default() }));
        let mut last: Option<[(i32, f64); 3]> = None;
        for mc in 4000..6000 {
            let g = v.siren.trio_gains(mc);
            let total: f64 = g.iter().map(|&(_, x)| if gains == Gains::Power { x * x } else { x }).sum();
            assert!((total - 1.0).abs() < 1e-12, "{gains:?} at {mc}: {total}");
            if let Some(prev) = last {
                // per note, the gain moves at most ~1 % per cent, across switches too
                for n in 30..80 {
                    let at = |s: &[(i32, f64); 3]| s.iter().find(|x| x.0 == n).map_or(0.0, |x| x.1);
                    let step = (at(&g) - at(&prev)).abs();
                    assert!(step < 0.012, "{gains:?} note {n} jumps by {step} at {mc}");
                }
            }
            last = Some(g);
        }
    }
}

#[test]
fn two_banks_on_a_note_play_the_plugin_note() {
    let lib = lib();
    let n = 44100;
    for siren in ["S1", "S4", "S7"] {
        let mut plugin = voice(&lib, siren, None);
        let linear = Trio { kernel: Kernel::Linear2, gains: Gains::Linear, ..Trio::default() };
        let mut trio = voice(&lib, siren, Some(linear));
        plugin.note_on(72, 127);
        trio.note_on(72, 127);
        let a: Vec<f32> = (0..n).map(|_| plugin.process()).collect();
        let b: Vec<f32> = (0..n).map(|_| trio.process()).collect();
        let db = snr_db(&a, &b);
        eprintln!("{siren}: two banks on the note against the plugin: {db:.1} dB");
        // the same partials; only f32 against f64 arithmetic differs
        assert!(db > 60.0, "{siren}: {db:.1} dB");
    }
}

#[test]
fn no_click_when_the_nearest_note_changes() {
    let lib = lib();
    for gains in [Gains::Linear, Gains::Power] {
        let mut v = voice(&lib, "S1", Some(Trio { gains, ..Trio::default() }));
        v.note_on(72, 127);
        for _ in 0..22050 {
            v.process();
        }
        // a slow sweep across 60.5 (where note 59 leaves and 62 enters)
        let mut out = Vec::new();
        let mut switches = Vec::new();
        let mut notes = v.siren.trio_state().map(|x| x.0);
        for mc in 5990..6110 {
            v.siren.jump_to(mc);
            for _ in 0..256 {
                out.push(v.process());
                let now = v.siren.trio_state().map(|x| x.0);
                if now != notes {
                    switches.push(out.len() - 1);
                    notes = now;
                }
            }
        }
        assert!(!switches.is_empty());
        let mut diffs: Vec<f32> = out.windows(2).map(|w| (w[1] - w[0]).abs()).collect();
        let at_switch = switches.iter().map(|&i| diffs[i.saturating_sub(1)].max(diffs[i.min(diffs.len() - 1)])).fold(0.0, f32::max);
        diffs.sort_by(f32::total_cmp);
        let p999 = diffs[diffs.len() * 999 / 1000];
        eprintln!("{gains:?}: largest step at a switch {at_switch:e}, 99.9th percentile of steps {p999:e}");
        assert!(at_switch <= p999 * 1.5, "{gains:?}: a step at the switch");
    }
}

#[test]
fn every_bank_walks_its_own_frames() {
    let lib = lib();
    let mut v = voice(&lib, "S1", Some(Trio::default()));
    v.siren.frames = FrameControl::Lab(LabFrames::full_loops(v.siren.data()));
    v.note_on(72, 127);
    v.siren.jump_to(6030);
    for _ in 0..44100 {
        v.process();
    }
    let st = v.siren.trio_state();
    assert_eq!(st.map(|x| x.0), [59, 60, 61]);
    for (n, _) in st {
        assert!(v.siren.frame_of(n as usize) > 0, "note {n} still on frame 0");
    }
    // the plugin's frames: the millisecond reset holds every bank on frame 0
    let mut v = voice(&lib, "S1", Some(Trio::default()));
    v.note_on(72, 127);
    v.siren.jump_to(6030);
    for _ in 0..44100 {
        v.process();
    }
    for (n, _) in v.siren.trio_state() {
        assert!(v.siren.frame_of(n as usize) <= 2, "note {n} got past frame 2 with the plugin's frames");
    }
}

#[test]
fn the_top_of_the_range_keeps_the_level() {
    // S1's last data note is 72: at 7200 the upper neighbour is missing
    let lib = lib();
    for edge in [Edge::Renormalize, Edge::Centre, Edge::Reflect] {
        let v = voice(&lib, "S1", Some(Trio { edge, ..Trio::default() }));
        let g = v.siren.trio_gains(7200);
        assert_eq!(g[2].1, 0.0, "{edge:?}: note 73 has no data");
        let power: f64 = g.iter().map(|x| x.1 * x.1).sum();
        assert!((power - 1.0).abs() < 1e-12, "{edge:?}: {power}");
    }
}

#[test]
fn sparse_data_plays_only_its_anchors() {
    let lib = lib();
    for kernel in [Kernel::BSpline3, Kernel::Linear2, Kernel::Nearest] {
        let t = Trio { kernel, gains: Gains::Linear, spacing: 4, origin: 2, ..Trio::default() };
        let v = voice(&lib, "S1", Some(t));
        for mc in 3000..7000 {
            let g = v.siren.trio_gains(mc);
            let total: f64 = g.iter().map(|x| x.1).sum();
            assert!((total - 1.0).abs() < 1e-12, "{kernel:?} at {mc}: {total}");
            for (n, w) in g {
                assert!(w == 0.0 || (n - 2).rem_euclid(4) == 0, "{kernel:?} at {mc}: note {n} is no anchor");
            }
            // the anchors around the target, each within 1.5 spacings
            for (n, w) in g {
                assert!(w == 0.0 || (mc as f64 / 100.0 - n as f64).abs() <= 6.0 + 1e-9, "{kernel:?} at {mc}: note {n}");
            }
        }
    }
}

#[test]
fn no_click_across_sparse_anchors() {
    let lib = lib();
    let t = Trio { gains: Gains::Linear, lock: true, spacing: 3, origin: 0, ..Trio::default() };
    let mut v = voice(&lib, "S1", Some(t));
    v.note_on(72, 127);
    for _ in 0..22050 {
        v.process();
    }
    // across 61.5 (anchors 57, 60, 63, 66: 57 leaves, 66 enters)
    let mut out = Vec::new();
    let mut switches = Vec::new();
    let mut notes = v.siren.trio_state().map(|x| x.0);
    for mc in 6100..6200 {
        v.siren.jump_to(mc);
        for _ in 0..256 {
            out.push(v.process());
            let now = v.siren.trio_state().map(|x| x.0);
            if now != notes {
                switches.push(out.len() - 1);
                notes = now;
            }
        }
    }
    assert!(!switches.is_empty());
    let mut diffs: Vec<f32> = out.windows(2).map(|w| (w[1] - w[0]).abs()).collect();
    let at_switch = switches.iter().map(|&i| diffs[i.saturating_sub(1)].max(diffs[i.min(diffs.len() - 1)])).fold(0.0, f32::max);
    diffs.sort_by(f32::total_cmp);
    let p999 = diffs[diffs.len() * 999 / 1000];
    assert!(at_switch <= p999 * 1.5, "a step at the switch: {at_switch:e} against {p999:e}");
}

#[test]
fn the_envelope_changes_nothing_on_an_anchor() {
    // ratio 1: E(f·1) / E(f) = 1 exactly, so the correction is the identity
    let lib = lib();
    let n = 22050;
    for siren in ["S1", "S7"] {
        let plain = Trio { kernel: Kernel::Nearest, gains: Gains::Linear, spacing: 3, ..Trio::default() };
        let mut a = voice(&lib, siren, Some(plain.clone()));
        let mut b = voice(&lib, siren, Some(Trio { envelope: 1.0, ..plain }));
        for v in [&mut a, &mut b] {
            v.note_on(72, 127);
            v.siren.jump_to(6000);
        }
        let x: Vec<f32> = (0..n).map(|_| a.process()).collect();
        let y: Vec<f32> = (0..n).map(|_| b.process()).collect();
        assert_eq!(x, y, "{siren}");
    }
}

/// `Trio::harmonic_envelope`: on an anchor (ratio 1) it changes nothing either;
/// between anchors it reads the envelope elsewhere than the slot's frequency
#[test]
fn the_envelope_at_the_harmonic() {
    let lib = lib();
    let n = 22050;
    let render = |mc: i32, at_harmonic: bool| {
        let t = Trio { kernel: Kernel::Nearest, gains: Gains::Linear, lock: true, spacing: 3, envelope: 1.0, harmonic_envelope: at_harmonic, ..Trio::default() };
        let mut v = voice(&lib, "S1", Some(t));
        v.note_on(72, 127);
        v.siren.jump_to(mc);
        (0..n).map(|_| v.process()).collect::<Vec<f32>>()
    };
    assert_eq!(render(6000, false), render(6000, true), "on an anchor");
    let (slot, harmonic) = (render(6100, false), render(6100, true));
    assert_ne!(slot, harmonic, "one semitone off an anchor");
    assert!(snr_db(&slot, &harmonic) > 10.0, "the same note, levels a little apart");
}

#[test]
fn the_envelope_goes_through_its_harmonics() {
    let lib = lib();
    let v = voice(&lib, "S1", Some(Trio { envelope: 1.0, ..Trio::default() }));
    let e = v.siren.envelope(60).expect("note 60 has an envelope");
    assert!(e.points.len() > 5);
    for &(h, d) in &e.points {
        assert!((e.at(h * e.f0) - d).abs() < 1e-9);
    }
    // flat past the ends, and between two harmonics within their levels
    let (first, last) = (e.points[0], e.points[e.points.len() - 1]);
    assert_eq!(e.at(first.0 * e.f0 / 2.0), first.1);
    assert_eq!(e.at(last.0 * e.f0 * 2.0), last.1);
    for w in e.points.windows(2) {
        let mid = e.at((w[0].0 * w[1].0).sqrt() * e.f0);
        assert!(mid >= w[0].1.min(w[1].1) - 1e-9 && mid <= w[0].1.max(w[1].1) + 1e-9);
    }
    let _: &Envelope = e;
}
