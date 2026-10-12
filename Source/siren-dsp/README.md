# siren-dsp

ComposeSiren's synthesis in Rust:

- `sirene`, `midi_in`, `siren`: the C++ DSP (`Source/ComposeSirenCore/lib/dsp/Sirene.cpp`,
  `MidiIn.cpp`) ported sample for sample. With `Timing::Plugin`, the default settings and the
  plugin's frame control, a voice is **bit-exact** with the C++ at any sample rate.
- the variants on the same voice: frame control that walks each note's frames (`FrameControl::Lab`,
  speed, interpolation, freeze), the trio of data notes around the pitch (`Trio`: kernels, gains,
  edges, tuning), and `layout` / `kernels`, a cache-friendly layout with faster partial sums.
- `settings`: `DspSettings`, every switch of the Rust voice keyed by id (`dsp.timing`,
  `frames.legacy`, `frames.speed`, `frames.interpolate`, `synth.crossfade`, `synth.bit16`,
  `synth.glide`, `synth.smoothing`, `trio.kernel`, `trio.gains`, `trio.edge`, `trio.scale_frames`,
  `trio.lock`, `trio.spacing`, `trio.origin`, `trio.envelope`, `trio.envelope_at_harmonic`), set
  from text and applied to a voice. The defaults are the plugin's behaviour (bit-exact).
- `variants`: the named variants, an engine and settings over the defaults:

  | id | engine | what |
  |---|---|---|
  | `cpp` | C++ | the plugin's DSP (the default) |
  | `rust-exact` | Rust | the port, bit-exact with `cpp` |
  | `rust-time` | Rust | timers and frames in seconds: the same at 44.1 kHz, right at other rates |
  | `frames` | Rust | each note loops over its frames (no plugin frame reset) |
  | `trio` | Rust | three data notes around the pitch, B-spline weights, phase-locked |
  | `trio-linear` | Rust | the two notes around the pitch, phase-locked |
  | `trio-nearest` | Rust | the nearest note alone, pitch-shifted |

  DSP variant and data set are app settings (global), not host-automatable parameters.
- `data`: the data sets through [`siren-data`](../siren-data) (raw `Resources/` or compact
  `.sirendata`, the same values).
- `ffi`: a C ABI (`sdsp_*`) for C, C++ or anything that calls C.

## In the plugin

Configure with `-DCOMPOSESIREN_RUST_DSP=ON` (default OFF). The plugin's voice
(`Source/ComposeSirenCore/lib/wrappers/SirenVoice`) then plays this crate's synthesis in place
of the C++ `Sirene` when the global `dsp.variant` setting names a Rust variant (`cpp`, the
default, keeps the C++ one), on the `dsp.dataset` data set: `<resources>/<dataset>/` when the
build ships it, else the resources folder. The plugin's C++ MIDI handling drives it, through
`include/siren_dsp_plugin.h` (`src/plugin.rs`); the data is loaded once per folder and shared by
every voice. Without the settings, `COMPOSESIREN_DSP_VARIANT` and `COMPOSESIREN_DSP_DATASET`
pick them.

### Changing the variant or the data set while playing

A change in Settings (or through MCP) applies live, in every instance of OneSiren and
SirenOrchestra and to every voice, without a restart (`SirenVoice::reloadDsp`):

- Each voice builds its new synthesis on its own loader thread, off the audio thread
  (loading every siren takes seconds with the C++ one), and publishes it with an atomic
  exchange. A newer request drops an older one still loading.
- The audio thread takes it just before the next note-on that finds no note held, so a
  change never cuts a held note, and a release only where the next note starts anyway. Until
  then the voice keeps the previous synthesis.
- At the swap the voice replays the last value of each controller (0 to 119) and the pitch
  wheel into the new synthesis, and fades the previous one out over 20 ms
  (`SirenVoice::fadeSeconds`), so a release still sounding doesn't click. The fade costs a
  second voice for 20 ms; a crossfade into the new note isn't needed, since it starts from
  silence. The next note starts from rest, as after loading a siren.
- The previous unit is then handed back and freed on the message thread (`collectGarbage`,
  from the plugins' timer). The audio thread takes no lock, allocates nothing and frees
  nothing.
- A siren change (another category, another resources folder) uses the same path but swaps
  at the next block, as before, now with the same 20 ms fade.

The choice is stored as before (the settings file), and the Settings window shows a change
made elsewhere (its rows are bound to the settings tree). The OFF build keeps the previous
voice, unchanged.

With `-DBUILD_TESTING=ON`, `ctest -R rust-dsp-parity` checks that `cpp` and `rust-exact` give
the same samples, bit for bit, through the voice (every siren, 44.1 and 48 kHz), and that the
other variants play.
`ctest -R dsp-hot-swap` plays a voice as a host would while the choice moves cpp -> rust-exact
-> trio-f32 -> trio -> cpp: each change is built off the audio thread, swaps only at a note-on
with no note held, the voice keeps playing, and after the swaps to rust-exact and cpp it plays
bit for bit what a fresh C++ voice plays (once the 20 ms fade is over).

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

## Performance

The Rust voices against the C++ voice (time per sample, load time, memory): [docs/dsp-performance.md](../../docs/dsp-performance.md), measured with `examples/perf.rs`.
