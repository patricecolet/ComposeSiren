//! Compiles the plugin's DSP sources as they are (no JUCE) with `oracle.cpp`, which drives a
//! voice the way `SirenVoiceUnit::process()` does.
use std::path::PathBuf;

fn main() {
    let here = PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    let core = here.join("../ComposeSirenCore").canonicalize().expect("Source/ComposeSirenCore");
    let sources = [
        "lib/dsp/Sirene.cpp",
        "lib/dsp/MidiIn.cpp",
        "lib/dsp/allpass.cpp",
        "lib/dsp/comb.cpp",
        "lib/dsp/mareverbe.cpp",
        "Reverb.cpp",
        "lib/definitions/sirenProperties.cpp",
        "lib/definitions/parameterDefinitions.cpp",
    ];
    let mut b = cc::Build::new();
    // -O2 for the tests; SIREN_ORACLE_OPT=3 for the plugin's Release flags (examples/perf.rs)
    let opt = std::env::var("SIREN_ORACLE_OPT").ok().and_then(|o| o.parse().ok()).unwrap_or(2);
    println!("cargo:rerun-if-env-changed=SIREN_ORACLE_OPT");
    b.cpp(true)
        .std("c++20")
        .opt_level(opt)
        .warnings(false)
        .include(&core)
        .include(core.join("lib"))
        .include(here.join("shim"))
        .file(here.join("oracle.cpp"));
    // Sirene.cpp calls memset without <cstring> (JUCE builds get it transitively)
    if b.get_compiler().is_like_msvc() {
        b.flag("/FIcstring");
    } else {
        b.flag("-include").flag("cstring");
    }
    for s in sources {
        let p = core.join(s);
        println!("cargo:rerun-if-changed={}", p.display());
        b.file(p);
    }
    for h in [
        "lib/dsp/Sirene.h",
        "lib/dsp/MidiIn.h",
        "lib/dsp/allpass.h",
        "lib/dsp/comb.h",
        "lib/dsp/mareverbe.h",
        "lib/dsp/rampe.h",
        "Reverb.h",
        "lib/definitions/sirenProperties.h",
        "lib/definitions/parameterDefinitions.h",
    ] {
        println!("cargo:rerun-if-changed={}", core.join(h).display());
    }
    println!("cargo:rerun-if-changed=shim/juce_dsp/juce_dsp.h");
    println!("cargo:rerun-if-changed=oracle.cpp");
    b.compile("composesiren_oracle");
}
