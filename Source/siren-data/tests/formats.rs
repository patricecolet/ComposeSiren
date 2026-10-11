//! Raw and compact give the same tables, for every data set of `Resources/` and of the fixed
//! data (`ResourcesProcessing/fix_data.fsx`), with the compact files written by the F# packer
//! (`ResourcesProcessing/compact.fsx`): the format is checked across the two languages.
//!
//! Needs the .NET SDK (`dotnet`) on the PATH. Every table cell is compared, including the zeros
//! past each note's data, and the raw view is checked against the file bytes as the C++ reads them.

use siren_data::{DataSet, DATA_SETS, FRAMES, NOTES, SLOTS};
use std::path::{Path, PathBuf};
use std::process::Command;

fn repo() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..").canonicalize().unwrap()
}

fn fsi(args: &[&str]) {
    let st = Command::new("dotnet")
        .arg("fsi")
        .args(args)
        .current_dir(repo().join("ResourcesProcessing"))
        .status()
        .expect("dotnet (the .NET SDK) is needed to write the compact files");
    assert!(st.success(), "dotnet fsi {args:?} failed");
}

fn scratch(what: &str) -> PathBuf {
    let d = Path::new(env!("CARGO_TARGET_TMPDIR")).join(what);
    let _ = std::fs::remove_dir_all(&d);
    d
}

/// The C++ view of a raw file: f32 little endian, table-sized, zero past the end.
fn cpp_table(path: &Path, len: usize) -> Vec<f32> {
    let bytes = std::fs::read(path).unwrap();
    let mut t = vec![0.0f32; len];
    for (i, c) in bytes.as_chunks::<4>().0.iter().take(len).enumerate() {
        t[i] = f32::from_le_bytes([c[0], c[1], c[2], c[3]]);
    }
    t
}

fn check_dir(raw_dir: &Path, compact_dir: &Path) {
    for name in DATA_SETS {
        let raw = DataSet::load_raw(raw_dir, name, name).unwrap();
        let compact = DataSet::load_compact(compact_dir, name, name).unwrap();
        let amp = cpp_table(&raw_dir.join(format!("dataAmp{name}")), NOTES * FRAMES * SLOTS);
        let freq = cpp_table(&raw_dir.join(format!("dataFreq{name}")), NOTES * FRAMES * SLOTS);
        let dure = cpp_table(&raw_dir.join(format!("datadureTabs{name}")), NOTES * 3);
        for n in 0..NOTES {
            assert_eq!(raw.kept_frames(n), compact.kept_frames(n), "{name} note {n} kept");
            for f in 0..FRAMES {
                for s in 0..SLOTS {
                    let i = (n * FRAMES + f) * SLOTS + s;
                    let (a, q) = (amp[i].to_bits(), freq[i].to_bits());
                    assert_eq!(raw.amp(n, f, s).to_bits(), a, "{name} raw amp [{n}][{f}][{s}]");
                    assert_eq!(compact.amp(n, f, s).to_bits(), a, "{name} compact amp [{n}][{f}][{s}]");
                    assert_eq!(raw.freq(n, f, s).to_bits(), q, "{name} raw freq [{n}][{f}][{s}]");
                    assert_eq!(compact.freq(n, f, s).to_bits(), q, "{name} compact freq [{n}][{f}][{s}]");
                }
            }
            for k in 0..3 {
                assert_eq!(raw.dure(n)[k].to_bits(), dure[n * 3 + k].to_bits(), "{name} dure");
                assert_eq!(compact.dure(n)[k].to_bits(), dure[n * 3 + k].to_bits(), "{name} compact dure");
            }
        }
        assert_eq!(raw.vector_interval(), compact.vector_interval());
        println!("{name}: {} notes, {} floats stored instead of {}", raw.file_notes(), raw.stored_floats(), 2 * NOTES * FRAMES * SLOTS);
    }
}

#[test]
fn raw_and_compact_agree_on_resources() {
    let resources = repo().join("Resources");
    let out = scratch("compact-resources");
    fsi(&["compact.fsx", "pack", resources.to_str().unwrap(), out.to_str().unwrap()]);
    check_dir(&resources, &out);
}

#[test]
fn raw_and_compact_agree_on_fixed_data() {
    let resources = repo().join("Resources");
    let fixed = scratch("fixed");
    let out = scratch("compact-fixed");
    fsi(&["fix_data.fsx", "--resources", resources.to_str().unwrap(), "--output", fixed.to_str().unwrap()]);
    fsi(&["compact.fsx", "pack", fixed.to_str().unwrap(), out.to_str().unwrap()]);
    check_dir(&fixed, &out);
}

#[test]
fn damaged_compact_file_is_refused() {
    let resources = repo().join("Resources");
    let out = scratch("compact-damaged");
    fsi(&["compact.fsx", "pack", resources.to_str().unwrap(), out.to_str().unwrap()]);
    let path = out.join("S7.sirendata");
    let mut bytes = std::fs::read(&path).unwrap();
    let mid = bytes.len() / 2;
    bytes[mid] ^= 1;
    let err = DataSet::from_compact("S7", &bytes).unwrap_err().to_string();
    assert!(err.contains("CRC"), "{err}");
}

#[test]
fn from_frames_rebuilds_a_set() {
    let res = repo().join("Resources");
    for name in DATA_SETS {
        let set = DataSet::load_raw(&res, name, name).unwrap();
        let mut dure = Vec::new();
        for n in 0..NOTES {
            dure.extend(set.dure(n));
        }
        let kept: Vec<usize> = (0..set.file_notes()).map(|n| set.kept_frames(n)).collect();
        let mut frames = Vec::new();
        for (n, &k) in kept.iter().enumerate() {
            for f in 0..k {
                frames.extend(set.frame_amp(n, f).unwrap().iter().flat_map(|x| x.to_le_bytes()));
            }
            for f in 0..k {
                frames.extend(set.frame_freq(n, f).unwrap().iter().flat_map(|x| x.to_le_bytes()));
            }
        }
        let back = DataSet::from_frames(name, &dure, &kept, set.file_notes(), &frames, *set.vector_interval()).unwrap();
        for n in 0..NOTES {
            assert_eq!(back.dure(n).map(f32::to_bits), set.dure(n).map(f32::to_bits));
            for f in 0..FRAMES {
                for s in 0..SLOTS {
                    assert_eq!(back.amp(n, f, s).to_bits(), set.amp(n, f, s).to_bits(), "{name} amp {n} {f} {s}");
                    assert_eq!(back.freq(n, f, s).to_bits(), set.freq(n, f, s).to_bits(), "{name} freq {n} {f} {s}");
                }
            }
        }
        assert_eq!(back.vector_interval(), set.vector_interval());
    }
}
