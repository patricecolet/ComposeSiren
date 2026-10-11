# siren-dsp

ComposeSiren's synthesis in Rust:

- `sirene`, `midi_in`, `siren`: the C++ DSP (`Source/ComposeSirenCore/lib/dsp/Sirene.cpp`,
  `MidiIn.cpp`) ported sample for sample. With `Timing::Plugin`, the default settings and the
  plugin's frame control, a voice is **bit-exact** with the C++ at any sample rate.
- the variants on the same voice: frame control that walks each note's frames (`FrameControl::Lab`,
  speed, interpolation, freeze), the trio of data notes around the pitch (`Trio`: kernels, gains,
  edges, tuning), and `layout` / `kernels`, a cache-friendly layout with faster partial sums.
- `data`: the data sets through [`siren-data`](../siren-data) (raw `Resources/` or compact
  `.sirendata`, the same values).
- `ffi`: a C ABI (`sdsp_*`) for C, C++ or anything that calls C.

## Tests

```sh
cargo test --release
```

The golden tests (`tests/oracle.rs`, `tests/midi_in.rs`) compare the port with the plugin's own C++,
compiled without JUCE by [`siren-dsp-oracle`](../siren-dsp-oracle) (a dev-dependency; its `build.rs`
uses the `cc` crate, so a C++20 compiler is needed). They compare every sample as bits: all seven
sirens, qualities 30 to 200, crossfade, 8/16-bit, 22.05 to 96 kHz, the whole MIDI path (CCs, pitch
wheel, vibrato, tremolo, portamento, attack, release) and the voice reading compact files. That last
case writes them with the F# packer, so it needs the .NET SDK.
