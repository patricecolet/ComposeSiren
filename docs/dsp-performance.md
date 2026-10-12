# DSP performance: the Rust voices against the C++ voice

Measured 2026-10-12 on one machine (the fast trio the same evening): Mac mini 2018, Intel Core i5-8500B 3.0 GHz, macOS 15.7.9,
rustc 1.98.1, Release builds. The C++ is the plugin's own voice sources, built by
`Source/siren-dsp-oracle` with the plugin's Release optimisation (`SIREN_ORACLE_OPT=3`); the
Rust voices are `Source/siren-dsp` with each named variant (`variants.rs`).

Reproduce from `Source/siren-dsp`:

```text
SIREN_ORACLE_OPT=3 cargo run --release --example perf                   # the tables below
cargo run --release --example perf -- rss <cpp|raw|compact> [dir]       # load time and memory
```

One voice at a time plays a held note with vibrato, rendered in host-sized blocks after a 0.5 s
warm-up. Each figure is the median of 7 runs of 2 s of audio: nanoseconds per sample, and in
brackets the real-time factor (how many such voices one core could play).

## Summary

- The default build (`COMPOSESIREN_RUST_DSP` OFF) has no regression: its voice and settings
  sources preprocess to the same code as before the Rust work (`763107c`), apart from one
  constant check in `Settings::isAvailable`, which isn't on the audio path.
- `rust-exact`, which plays the same samples as the C++ bit for bit, now costs about 0.9× the
  C++ (S3 at 48 kHz: 198 against 220 ns per sample; 1211 against 1454 at 200 partials). It used to cost
  about 1.6× (352 ns per sample). The gain is from reading each note's frame once per sample
  instead of once per partial, with the arithmetic unchanged, so the parity tests still pass.
  `rust-time` and `frames` cost the same as `rust-exact`.
- The trio variants cost about 5× the C++ (S3: 1041 ns per sample, 20 voices per core at 30
  partials; 1720 before the frame lookups were hoisted). They play up to four banks of
  partials, each with its own libm `sin` per partial per sample.
- The fast trio variants (`trio-fast`, `trio-linear-fast`, `trio-nearest-fast`: `trio.fast`)
  cost about 2.2× the C++, 2.2 to 2.9× faster than the trio they speed up (S3: 474 against
  1041 ns per sample at 30 partials, 2544 against 7360 at 200; 44 voices per core). Their
  sines come from `fastsin.rs`, a branch-free polynomial the compiler vectorizes (SSE2/AVX on
  x86_64, NEON on aarch64; plain Rust, no intrinsics, so the same code is the scalar
  fallback), and a locked phase is only formed on the samples that are computed. Not bit for
  bit: `tests/fast.rs` checks the sine within 1e-9 of libm's (measured 6.6e-10) and every
  siren within 1e-6 of the peak of the trio it speeds up (measured 1.1e-7, the f32 output's
  rounding). The lab's phasor kernels (`kernels.rs`, f32 phasors re-anchored per frame) would
  be faster still, but drift further from this voice (bounded per frame, about −46 dB
  before re-anchoring); not used.
- Block size, from 64 to 512 samples, changes nothing; time per sample doesn't depend on the
  sample rate, so the real-time factor halves at 96 kHz.
- Loading: the C++ loader takes 7.0 s and 854 MiB for every siren's data. The Rust raw loader
  takes 0.5 s and 300 MiB, and the compact `.sirendata` files take 0.18 s and 94 MiB.

## Load time and memory

Load every siren's data once and measure the growth in resident memory (median of 3 runs).

| loader | data | time | memory |
|---|---|---:|---:|
| C++ (`cpp`) | raw `Resources/` | 7.0 s | 854 MiB |
| Rust `siren-data` | raw `Resources/` | 0.50 s | 301 MiB |
| Rust `siren-data` | compact `.sirendata` (57 MB on disk) | 0.18 s | 94 MiB |

## Time per sample

### Each siren (48 kHz, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| S1 | 138 (151x) | 134 (155x) | 133 (156x) | 133 (157x) | 985 (21x) | 988 (21x) | 984 (21x) | 455 (46x) | 453 (46x) | 453 (46x) |
| S2 | 137 (152x) | 133 (157x) | 132 (158x) | 133 (156x) | 987 (21x) | 985 (21x) | 985 (21x) | 454 (46x) | 454 (46x) | 456 (46x) |
| S3 | 211 (99x) | 202 (103x) | 201 (104x) | 203 (103x) | 1042 (20x) | 1036 (20x) | 1038 (20x) | 468 (45x) | 467 (45x) | 467 (45x) |
| S4 | 228 (91x) | 220 (95x) | 218 (95x) | 220 (95x) | 1070 (19x) | 1062 (20x) | 1067 (20x) | 469 (44x) | 468 (45x) | 469 (44x) |
| S5 | 286 (73x) | 274 (76x) | 273 (76x) | 278 (75x) | 1150 (18x) | 1175 (18x) | 1164 (18x) | 478 (44x) | 479 (44x) | 481 (43x) |
| S6 | 289 (72x) | 277 (75x) | 273 (76x) | 279 (75x) | 1154 (18x) | 1152 (18x) | 1154 (18x) | 478 (44x) | 475 (44x) | 475 (44x) |
| S7 | 255 (82x) | 243 (86x) | 242 (86x) | 249 (84x) | 1258 (17x) | 1254 (17x) | 1261 (17x) | 471 (44x) | 471 (44x) | 471 (44x) |

### Sample rates (S3, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 44.1 kHz | 211 (108x) | 204 (111x) | 203 (112x) | 204 (111x) | 1037 (22x) | 1034 (22x) | 1038 (22x) | 465 (49x) | 464 (49x) | 465 (49x) |
| 48 kHz | 210 (99x) | 202 (103x) | 203 (103x) | 203 (102x) | 1039 (20x) | 1051 (20x) | 1049 (20x) | 478 (44x) | 471 (44x) | 467 (45x) |
| 96 kHz | 205 (51x) | 198 (53x) | 194 (54x) | 196 (53x) | 1028 (10x) | 1025 (10x) | 1032 (10x) | 465 (22x) | 465 (22x) | 466 (22x) |

### Quality (S3, 48 kHz, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 30 partials | 209 (100x) | 201 (103x) | 201 (104x) | 203 (103x) | 1041 (20x) | 1038 (20x) | 1046 (20x) | 474 (44x) | 465 (45x) | 467 (45x) |
| 120 partials | 884 (24x) | 798 (26x) | 795 (26x) | 798 (26x) | 4132 (5x) | 4125 (5x) | 4107 (5x) | 1556 (13x) | 1558 (13x) | 1558 (13x) |
| 200 partials | 1377 (15x) | 1253 (17x) | 1231 (17x) | 1283 (16x) | 7360 (3x) | 7333 (3x) | 7530 (3x) | 2544 (8x) | 2559 (8x) | 2576 (8x) |

### Block size (S3, 48 kHz, 30 partials)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 samples | 209 (100x) | 202 (103x) | 200 (104x) | 203 (103x) | 1042 (20x) | 1037 (20x) | 1038 (20x) | 470 (44x) | 475 (44x) | 479 (43x) |
| 128 samples | 209 (99x) | 203 (103x) | 204 (102x) | 206 (101x) | 1051 (20x) | 1042 (20x) | 1053 (20x) | 482 (43x) | 481 (43x) | 480 (43x) |
| 256 samples | 209 (100x) | 203 (103x) | 202 (103x) | 207 (101x) | 1043 (20x) | 1039 (20x) | 1040 (20x) | 469 (44x) | 470 (44x) | 466 (45x) |
| 512 samples | 209 (99x) | 202 (103x) | 201 (104x) | 204 (102x) | 1043 (20x) | 1044 (20x) | 1039 (20x) | 469 (44x) | 469 (44x) | 473 (44x) |

## Before the fast trio (764d194)

### Each siren (48 kHz, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| S1 | 142 (147x) | 128 (163x) | 127 (164x) | 130 (161x) | 1041 (20x) | 1035 (20x) | 1166 (18x) |
| S2 | 149 (139x) | 131 (159x) | 127 (164x) | 132 (158x) | 1098 (19x) | 1083 (19x) | 1080 (19x) |
| S3 | 221 (94x) | 198 (105x) | 200 (104x) | 208 (100x) | 1139 (18x) | 1125 (19x) | 1125 (19x) |
| S4 | 244 (85x) | 223 (93x) | 214 (98x) | 217 (96x) | 1131 (18x) | 1153 (18x) | 1248 (17x) |
| S5 | 306 (68x) | 272 (77x) | 273 (76x) | 279 (75x) | 1255 (17x) | 1256 (17x) | 1212 (17x) |
| S6 | 304 (69x) | 271 (77x) | 271 (77x) | 278 (75x) | 1203 (17x) | 1197 (17x) | 1203 (17x) |
| S7 | 270 (77x) | 241 (87x) | 240 (87x) | 245 (85x) | 1314 (16x) | 1306 (16x) | 1325 (16x) |

### Sample rates (S3, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| 44.1 kHz | 223 (102x) | 202 (112x) | 199 (114x) | 201 (113x) | 1105 (21x) | 1094 (21x) | 1092 (21x) |
| 48 kHz | 220 (94x) | 199 (105x) | 198 (105x) | 205 (102x) | 1124 (19x) | 1115 (19x) | 1093 (19x) |
| 96 kHz | 214 (49x) | 193 (54x) | 188 (55x) | 193 (54x) | 1090 (10x) | 1089 (10x) | 1092 (10x) |

### Quality (S3, 48 kHz, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| 30 partials | 219 (95x) | 199 (105x) | 197 (106x) | 202 (103x) | 1117 (19x) | 1100 (19x) | 1106 (19x) |
| 120 partials | 951 (22x) | 808 (26x) | 799 (26x) | 802 (26x) | 4396 (5x) | 4389 (5x) | 4388 (5x) |
| 200 partials | 1454 (14x) | 1211 (17x) | 1222 (17x) | 1289 (16x) | 7775 (3x) | 7779 (3x) | 7729 (3x) |

### Block size (S3, 48 kHz, 30 partials)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| 64 samples | 219 (95x) | 197 (106x) | 196 (106x) | 200 (104x) | 1097 (19x) | 1100 (19x) | 1104 (19x) |
| 128 samples | 218 (96x) | 197 (106x) | 198 (105x) | 200 (104x) | 1090 (19x) | 1102 (19x) | 1108 (19x) |
| 256 samples | 220 (95x) | 198 (105x) | 197 (106x) | 201 (104x) | 1091 (19x) | 1089 (19x) | 1103 (19x) |
| 512 samples | 219 (95x) | 198 (105x) | 200 (104x) | 204 (102x) | 1103 (19x) | 1098 (19x) | 1103 (19x) |

## Before the frame lookups were hoisted (1100c9e)

The same tables on the same machine before `25fc802`.
### Each siren (48 kHz, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| S1 | 141 (148x) | 238 (88x) | 236 (88x) | 243 (86x) | 1557 (13x) | 1551 (13x) | 1560 (13x) |
| S2 | 141 (148x) | 238 (88x) | 236 (88x) | 243 (86x) | 1550 (13x) | 1556 (13x) | 1558 (13x) |
| S3 | 217 (96x) | 351 (59x) | 350 (60x) | 356 (59x) | 1720 (12x) | 1714 (12x) | 1718 (12x) |
| S4 | 239 (87x) | 375 (56x) | 375 (56x) | 380 (55x) | 1769 (12x) | 1768 (12x) | 1843 (11x) |
| S5 | 305 (68x) | 471 (44x) | 484 (43x) | 467 (45x) | 1979 (11x) | 1973 (11x) | 1994 (10x) |
| S6 | 303 (69x) | 455 (46x) | 454 (46x) | 459 (45x) | 1914 (11x) | 1906 (11x) | 1911 (11x) |
| S7 | 269 (77x) | 407 (51x) | 405 (51x) | 413 (50x) | 1972 (11x) | 1996 (10x) | 1978 (11x) |

### Sample rates (S3, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| 44.1 kHz | 219 (104x) | 353 (64x) | 356 (64x) | 356 (64x) | 1717 (13x) | 1714 (13x) | 1716 (13x) |
| 48 kHz | 217 (96x) | 352 (59x) | 350 (59x) | 357 (58x) | 1729 (12x) | 1714 (12x) | 1721 (12x) |
| 96 kHz | 212 (49x) | 351 (30x) | 343 (30x) | 350 (30x) | 1710 (6x) | 1703 (6x) | 1727 (6x) |

### Quality (S3, 48 kHz, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| 30 partials | 220 (95x) | 353 (59x) | 353 (59x) | 359 (58x) | 1729 (12x) | 1713 (12x) | 1720 (12x) |
| 120 partials | 930 (22x) | 1406 (15x) | 1404 (15x) | 1486 (14x) | 6891 (3x) | 6890 (3x) | 6869 (3x) |
| 200 partials | 1447 (14x) | 2218 (9x) | 2215 (9x) | 2307 (9x) | 11873 (2x) | 12092 (2x) | 12139 (2x) |

### Block size (S3, 48 kHz, 30 partials)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest |
|---|---:|---:|---:|---:|---:|---:|---:|
| 64 samples | 219 (95x) | 356 (59x) | 356 (59x) | 359 (58x) | 1744 (12x) | 1741 (12x) | 1734 (12x) |
| 128 samples | 223 (93x) | 359 (58x) | 354 (59x) | 359 (58x) | 1739 (12x) | 1719 (12x) | 1764 (12x) |
| 256 samples | 219 (95x) | 355 (59x) | 354 (59x) | 362 (58x) | 1746 (12x) | 1747 (12x) | 1738 (12x) |
| 512 samples | 218 (96x) | 356 (59x) | 352 (59x) | 357 (58x) | 1744 (12x) | 1733 (12x) | 1735 (12x) |
