# siren-dsp-oracle

The reference for [`siren-dsp`](../siren-dsp)'s golden tests: the plugin's own DSP sources
(`Sirene.cpp`, `MidiIn.cpp`, the siren and parameter definitions), compiled as they are without JUCE
by `build.rs` (the `cc` crate), with `oracle.cpp` driving one voice the way
`SirenVoiceUnit::process()` does. `CppVoice` is the safe Rust handle. Test-only: nothing ships it.
