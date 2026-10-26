# reverb-dsp

ComposeSiren's stereo reverb in Rust, extracted as a standalone zero-dependency crate so
programs and tools can run the synthesis (`siren-dsp`) and reverb (`reverb-dsp`) directly
without hosting the full plugin:

- `mareverbe`: the Freeverb stereo core (`Source/ComposeSirenCore/lib/dsp/mareverbe.cpp`,
  `comb.cpp`, `allpass.cpp`) ported sample for sample (`Mareverbe`, `Comb`, `Allpass`,
  `dsp_clip`).
- `biquad`: the 2nd-order Butterworth low-pass and high-pass filters (`IirFilter`,
  `IirCoefficients`) matching `juce::IIRFilter` / `juce::IIRCoefficients`.
- `rampe`: the linear parameter ramp (`Rampe`) matching `Source/ComposeSirenCore/lib/dsp/rampe.h`.
- `reverb`: the complete `Reverb` processor (`Source/ComposeSirenCore/Reverb.cpp`). With
  `ReverbMode::Plugin` (the default), `Reverb` is **bit-exact** with the C++ implementation
  at any sample rate (both for separate input/output buffers and for in-place buffers).
  `ReverbMode::StereoTime` provides independent stereo high-pass filtering, sample-rate-scaled
  ramps, and non-aliasing dry/wet mixing on in-place buffers.
- `ffi`: C ABI (`csr_reverb_*`, `csr_mareverbe_*`) declared in `include/reverb_dsp.h`.

## In the plugin

When configured with `-DCOMPOSESIREN_RUST_DSP=ON`, `Source/ComposeSirenCore/Reverb` delegates
to `reverb-dsp` through `include/reverb_dsp.h` whenever a Rust DSP variant (`dsp.variant != "cpp"`)
is selected, and uses the C++ implementation when `dsp.variant` is `"cpp"` (or when built with
`COMPOSESIREN_RUST_DSP=OFF`).

## Tests

```sh
cargo test --release
```

`tests/oracle.rs` compares `Mareverbe` and `Reverb` against the C++ implementation compiled by
[`siren-dsp-oracle`](../siren-dsp-oracle), verifying bit-for-bit equality (`a.to_bits() == b.to_bits()`)
across sample rates (22.05 to 96 kHz), parameter sets, filter cutoffs, MIDI CC changes,
enable/disable transitions, freeze mode, and both out-of-place and in-place buffer processing.
