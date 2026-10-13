//! `frames.skip_silent`: the practically silent frames of the data, and what skipping them does.

use siren_dsp::Library;
use siren_dsp::data::{silent_frames, SILENT_REL_DB};
use siren_dsp::siren::{SIRENS, SirenId};
use siren_dsp::sirene::{FrameControl, LabFrames};

fn lib() -> Library {
    Library::new(siren_dsp::default_resources())
}

/// S5's F5 (data note 77, the plugin's MIDI 89; S6 plays S5's data), frame 440 of 547: the one
/// frame of the data this far under its note (-42.7 dB), heard as a click when the loop gets there.
const NOTE: usize = 77;
const FRAME: usize = 440;

#[test]
fn the_data_has_one_silent_frame_s5_f5_440() {
    let lib = lib();
    let mut found = Vec::new();
    for def in SIRENS.iter() {
        let data = lib.data(def).unwrap();
        for i in &data.info {
            for (f, s) in silent_frames(&i.frame_energy, i.fmax, i.data_frames).iter().enumerate() {
                if *s && f < i.fmax {
                    found.push((def.name(), i.index, f));
                }
            }
            for f in 0..i.frames {
                let g = i.skip_to[f] as usize;
                assert!(g < i.frames, "{} n{} f{f} -> {g}", def.name(), i.index);
            }
        }
    }
    eprintln!("silent frames under {SILENT_REL_DB} dB: {found:?}");
    assert!(found.iter().all(|&(_, n, f)| n == NOTE && f == FRAME), "{found:?}");
    assert!(found.iter().any(|&(s, _, _)| s == "S5"), "{found:?}");
    let data = lib.data(SirenId::S6.def()).unwrap();
    assert_eq!(data.play_frame(NOTE, FRAME, true), FRAME + 1);
    assert_eq!(data.play_frame(NOTE, FRAME, false), FRAME);
    assert_eq!(data.play_frame(NOTE, FRAME - 1, true), FRAME - 1);
}

/// The note looped over its frames (the lab's frames, as rust-time and the trio variants play),
/// with frames read as `skip` says; the frame each sample was counted on.
fn render(id: SirenId, note: usize, skip: bool, interpolate: bool, seconds: f64) -> (Vec<f32>, Vec<usize>) {
    let lib = lib();
    let mut v = lib.voice(id, 44100.0).unwrap();
    v.glide = false;
    let mut l = LabFrames::full_loops(v.siren.data());
    l.interpolate = interpolate;
    v.siren.frames = FrameControl::Lab(l);
    v.siren.set_16ou8bit(false);
    v.siren.skip_silent = skip;
    v.note_on(note as i32 + 12, 127);
    let n = (seconds * 44100.0) as usize;
    let (mut out, mut frames) = (Vec::with_capacity(n), Vec::with_capacity(n));
    for _ in 0..n {
        out.push(v.process());
        frames.push(v.siren.frame_of(note));
    }
    (out, frames)
}

/// The click: the peak RMS of the signal's second difference (what a jump or a kink in it
/// leaves) over 128-sample windows, in dB over the median window, once the note has settled.
fn click_db(x: &[f32]) -> f64 {
    let d: Vec<f32> = x.windows(3).map(|w| w[2] - 2.0 * w[1] + w[0]).collect();
    let w = 128;
    let mut rms: Vec<f64> = d[22050..]
        .chunks_exact(w)
        .map(|c| (c.iter().map(|&s| (s as f64).powi(2)).sum::<f64>() / w as f64).sqrt())
        .collect();
    let max = rms.iter().copied().fold(0.0, f64::max);
    rms.sort_by(f64::total_cmp);
    20.0 * (max / rms[rms.len() / 2]).log10()
}

#[test]
fn skipping_takes_the_click_out_of_s6_f5() {
    let lib = lib();
    let info = lib.data(SirenId::S6.def()).unwrap().info[NOTE].clone();
    let seconds = 1.0 + info.fdur as f64 * info.fmax as f64 / 44100.0;
    for interpolate in [false, true] {
        let (off, frames) = render(SirenId::S6, NOTE, false, interpolate, seconds);
        let (on, _) = render(SirenId::S6, NOTE, true, interpolate, seconds);
        assert!(frames.contains(&FRAME), "the loop reaches frame {FRAME}");
        let (c_off, c_on) = (click_db(&off), click_db(&on));
        eprintln!("S6 F5 (fdur {}, fmax {}), interpolate {interpolate}: click off {c_off:.1} dB, on {c_on:.1} dB", info.fdur, info.fmax);
        assert!(c_off > 12.0, "the click is there with skipping off: {c_off:.1} dB");
        assert!(c_on < 6.0, "skipping takes it out: {c_on:.1} dB");
        // until the loop gets to the silent frame (or the one read towards it), not a sample changes
        let first = frames.iter().position(|&f| f + 1 == FRAME).unwrap();
        assert!(off[..first].iter().zip(&on[..first]).all(|(a, b)| a.to_bits() == b.to_bits()));
    }
}

#[test]
fn skipping_changes_nothing_where_no_frame_is_silent() {
    for (id, note) in [(SirenId::S1, 60), (SirenId::S3, 50), (SirenId::S7, 70)] {
        for interpolate in [false, true] {
            let (off, _) = render(id, note, false, interpolate, 1.5);
            let (on, _) = render(id, note, true, interpolate, 1.5);
            assert!(off.iter().zip(&on).all(|(a, b)| a.to_bits() == b.to_bits()), "{id:?} n{note}");
        }
    }
}

#[test]
fn the_plugin_frames_with_skipping_on_are_the_plugins() {
    // the plugin's frame reset never leaves the first frames: nothing to skip there
    let lib = lib();
    for skip in [false, true] {
        let mut v = lib.voice(SirenId::S6, 44100.0).unwrap();
        v.siren.skip_silent = skip;
        v.note_on(NOTE as i32 + 12, 127);
        let x: Vec<f32> = (0..44100).map(|_| v.process()).collect();
        let mut w = lib.voice(SirenId::S6, 44100.0).unwrap();
        w.note_on(NOTE as i32 + 12, 127);
        let y: Vec<f32> = (0..44100).map(|_| w.process()).collect();
        assert!(x.iter().zip(&y).all(|(a, b)| a.to_bits() == b.to_bits()));
    }
}

#[test]
fn every_rust_variant_skips_when_asked() {
    use siren_dsp::settings::Engine;
    use siren_dsp::variants::VARIANTS;
    let lib = lib();
    let info = lib.data(SirenId::S6.def()).unwrap().info[NOTE].clone();
    let seconds = 1.0 + info.fdur as f64 * info.fmax as f64 / 44100.0;
    let n = (seconds * 44100.0) as usize;
    let mut report = Vec::new();
    for var in VARIANTS.iter().filter(|v| v.engine == Engine::Rust) {
        let mut st = var.dsp_settings().unwrap();
        let mut play = |skip: bool| -> Vec<f32> {
            st.skip_silent = skip;
            let mut v = lib.voice(SirenId::S6, 44100.0).unwrap();
            v.siren.set_timing(st.timing);
            st.apply_synthesis(&mut v.siren);
            assert_eq!(v.siren.skip_silent, skip);
            v.glide = st.glide;
            v.note_on(NOTE as i32 + 12, 127);
            (0..n).map(|_| v.process()).collect()
        };
        let off = play(false);
        let on = play(true);
        let frames_legacy = st.frames_legacy;
        let (c_off, c_on) = (click_db(&off), click_db(&on));
        report.push(format!("{:<18} legacy {:<5} click off {c_off:5.1} dB, on {c_on:5.1} dB", var.id, frames_legacy));
        if frames_legacy {
            // never past the first frames: skipping is a no-op, bit for bit
            assert!(off.iter().zip(&on).all(|(a, b)| a.to_bits() == b.to_bits()), "{}", var.id);
        } else {
            assert!(c_on < 6.0 && c_on < c_off - 3.0, "{}: off {c_off:.1}, on {c_on:.1}", var.id);
        }
    }
    eprintln!("S6 F5, every Rust variant:\n{}", report.join("\n"));
}
