//! Which frames a held note plays: the plugin's, and the lab's.

use std::collections::BTreeSet;

use siren_dsp::Library;
use siren_dsp::siren::{SIRENS, SirenId};
use siren_dsp::sirene::{FrameControl, LabFrames};

fn lib() -> Library {
    Library::new(siren_dsp::default_resources())
}

/// Frames visited by the note's own counter over `seconds`, after the pitch
/// has settled (glide off: the voice is on the note from the first sample).
fn visited(lib: &Library, id: SirenId, note: usize, lab: bool, seconds: f64) -> BTreeSet<usize> {
    let mut v = lib.voice(id, 44100.0).unwrap();
    v.glide = false;
    if lab {
        v.siren.frames = FrameControl::Lab(LabFrames::full_loops(v.siren.data()));
    }
    v.note_on(note as i32 + 12, 127);
    let mut seen = BTreeSet::new();
    for _ in 0..(seconds * 44100.0) as usize {
        v.process();
        seen.insert(v.siren.frame_of(note));
    }
    seen
}

#[test]
fn the_plugin_never_gets_past_the_first_frames() {
    let lib = lib();
    let mut report = Vec::new();
    for def in SIRENS.iter().filter(|d| matches!(d.id, SirenId::S1 | SirenId::S3 | SirenId::S4 | SirenId::S5 | SirenId::S7)) {
        let data = lib.data(def).unwrap();
        let top = def.constants.note_midi_cent_max as usize / 100;
        for note in (def.constants.note_min as usize..=top).step_by(6) {
            let info = &data.info[note];
            if info.fmax < 2 {
                continue;
            }
            let seen = visited(&lib, def.id, note, false, 2.0);
            // setMidicent() runs every 44 samples and puts the counter back
            // to 0: a frame of fdur samples is left only if fdur < 44
            let reachable = 44usize.div_ceil(info.fdur as usize).min(info.fmax);
            assert!(seen.len() <= reachable.max(1), "{} note {note}: {seen:?}", def.name());
            report.push(format!("{} n{note} fdur {} fmax {}: plays {:?}", def.name(), info.fdur, info.fmax, seen));

            let lab = visited(&lib, def.id, note, true, 2.0 + info.fdur as f64 * info.fmax as f64 / 44100.0);
            assert_eq!(lab, (0..info.fmax).collect(), "{} note {note}: the lab loop", def.name());
        }
    }
    eprintln!("{}", report.join("\n"));
}
