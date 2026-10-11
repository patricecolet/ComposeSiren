//! Compiles the plugin's DSP sources as they are (no JUCE) with `oracle.cpp`, which drives a
//! voice the way `SirenVoiceUnit::process()` does.
use std::path::PathBuf;

fn main() {
    let here = PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    let core = here.join("../ComposeSirenCore").canonicalize().expect("Source/ComposeSirenCore");
    let sources = [
        "lib/dsp/Sirene.cpp",
        "lib/dsp/MidiIn.cpp",
        "lib/definitions/sirenProperties.cpp",
        "lib/definitions/parameterDefinitions.cpp",
    ];
    let mut b = cc::Build::new();
    b.cpp(true).std("c++20").opt_level(2).warnings(false).include(&core).include(core.join("lib")).file(here.join("oracle.cpp"));
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
    for h in ["lib/dsp/Sirene.h", "lib/dsp/MidiIn.h", "lib/definitions/sirenProperties.h", "lib/definitions/parameterDefinitions.h"] {
        println!("cargo:rerun-if-changed={}", core.join(h).display());
    }
    println!("cargo:rerun-if-changed=oracle.cpp");
    b.compile("composesiren_oracle");
}
