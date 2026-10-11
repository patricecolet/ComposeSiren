//! The port against the plugin's own C++ (`siren-dsp-oracle`), sample by sample, bit for bit:
//! from the raw files, and with the Rust side reading the compact format (written by the F#
//! packer, so this needs the .NET SDK).

use std::path::{Path, PathBuf};
use std::process::Command;
use siren_dsp_oracle::CppVoice;

use siren_dsp::Library;
use siren_dsp::siren::SirenId;
use siren_dsp::sirene::Timing;

fn resources() -> PathBuf {
    siren_dsp::default_resources()
}

/// `Resources/` packed by `ResourcesProcessing/compact.fsx`, once per test run.
fn compact_resources() -> PathBuf {
    let out = Path::new(env!("CARGO_TARGET_TMPDIR")).join("compact-resources");
    let st = Command::new("dotnet")
        .args(["fsi", "compact.fsx", "pack"])
        .arg(resources())
        .arg(&out)
        .current_dir(Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ResourcesProcessing"))
        .status()
        .expect("dotnet (the .NET SDK) is needed to write the compact files");
    assert!(st.success());
    out
}

struct Case {
    siren: &'static str,
    midi_note: i32,
    velocity: i32,
    samples: usize,
    quality: i32,
    crossfade: bool,
    bit16: bool,
    sample_rate: f64,
}

fn check(lib: &Library, c: &Case) {
    let mut cpp = CppVoice::new(&resources(), c.siren, c.sample_rate).expect("the C++ voice");
    cpp.set_quality(c.quality);
    cpp.set_crossfade(c.crossfade);
    cpp.set_8bit(!c.bit16);
    cpp.note_on(c.midi_note, c.velocity);
    let mut want = vec![0.0f32; c.samples];
    cpp.render(&mut want);

    let mut v = lib.voice(SirenId::parse(c.siren).unwrap(), c.sample_rate).unwrap();
    // the plugin counts its timers and frames in samples (the same as Time at 44.1 kHz)
    v.set_timing(Timing::Plugin);
    v.siren.change_qualite(c.quality);
    v.siren.set_crossfade(c.crossfade);
    v.siren.set_16ou8bit(!c.bit16);
    v.note_on(c.midi_note, c.velocity);
    let got: Vec<f32> = (0..c.samples).map(|_| v.process()).collect();

    let first = got.iter().zip(&want).position(|(a, b)| a.to_bits() != b.to_bits());
    let max = got.iter().zip(&want).map(|(a, b)| (a - b).abs()).fold(0.0f32, f32::max);
    let peak = want.iter().fold(0.0f32, |m, x| m.max(x.abs()));
    eprintln!(
        "{} note {} q{} xf{} 16b{} @{}: peak {peak:.4}, max |diff| {max:e}, first differing sample {first:?}",
        c.siren, c.midi_note, c.quality, c.crossfade as u8, c.bit16 as u8, c.sample_rate
    );
    assert!(peak > 0.01, "the oracle rendered silence");
    assert_eq!(first, None, "the port differs from the C++");
}

#[test]
fn port_matches_the_plugin_sample_for_sample() {
    let lib = Library::new(resources());
    // the rotor spins up from 0 for 1 to 4 s: cover that and the steady note
    let seconds = 8 * 44100;
    for (siren, midi_note) in [("S1", 72), ("S2", 60), ("S3", 66), ("S4", 70), ("S5", 84), ("S6", 79), ("S7", 88)] {
        check(&lib, &Case { siren, midi_note, velocity: 127, samples: seconds, quality: 30, crossfade: false, bit16: false, sample_rate: 44100.0 });
    }
    for (quality, crossfade, bit16) in [(200, false, false), (30, true, false), (30, false, true), (120, true, true)] {
        check(&lib, &Case { siren: "S1", midi_note: 66, velocity: 100, samples: seconds, quality, crossfade, bit16, sample_rate: 44100.0 });
    }
}

/// `Timing::Plugin` is the plugin at any rate, its 9 % fast timers included
#[test]
fn plugin_timing_matches_the_plugin_at_other_rates() {
    let lib = Library::new(resources());
    for (siren, midi_note, sample_rate) in [("S1", 72, 48000.0), ("S7", 88, 96000.0), ("S4", 70, 22050.0)] {
        let samples = 4 * sample_rate as usize;
        check(&lib, &Case { siren, midi_note, velocity: 127, samples, quality: 30, crossfade: true, bit16: false, sample_rate });
    }
}

/// The Rust voice on the compact files sounds bit for bit as the C++ on the raw ones.
#[test]
fn port_on_compact_data_matches_the_plugin() {
    let lib = Library::new(compact_resources());
    for (siren, midi_note) in [("S1", 72), ("S4", 70), ("S7", 88)] {
        check(&lib, &Case { siren, midi_note, velocity: 127, samples: 4 * 44100, quality: 30, crossfade: true, bit16: false, sample_rate: 44100.0 });
    }
}
