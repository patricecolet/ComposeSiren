# siren-data

Loads ComposeSiren's siren data sets as the C++ DSP (`Sirene::readDataFromBinaryFile`) sees them:
`[80][1000][200]` amplitude and frequency tables, `[80][3]` note durations and 392 rotor inertia
rates, zero wherever a file is shorter. It reads either source:

- raw files (`Resources/`, or the build's `Resources-processed/`),
- compact files (`SN.sirendata`, [format](../../ResourcesProcessing/compact-format.md)).

Only each note's frames up to its last non-zero one are kept in memory (≈ 57 MB for the five
original data sets instead of 603 MB; the crossfade-fixed data fills every frame with a
frequency, so it stays at full size). Accessors do not allocate.

```sh
cargo test --release   # needs the .NET SDK: the compact files come from the F# packer
```

The tests compare every table cell (as bits) of the raw and compact loads with the bytes the C++
reads, for `Resources/` and for the output of `ResourcesProcessing/fix_data.fsx`.
