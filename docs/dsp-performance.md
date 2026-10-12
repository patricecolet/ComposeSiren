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
  re-anchor per frame and can't follow the voice's glide, which changes the pitch every
  sample; the f32 variants below take their idea instead.
- The f32 phasor trio variants (`trio-f32`, `trio-linear-f32`, `trio-nearest-f32`:
  `trio.fast` and `trio.f32`) cost about 1.6× the C++ at 30 partials and 1.3× at 200 (S3: 328
  against 446 for the fast trio and 1056 for the trio; 1780 against 2257 and 7453 at 200), 64
  voices per core at 30 partials. With the phase lock, every bank's partial sounds harmonic h on
  h·Φ, so one f32 polynomial cos/sin of Φ per computed sample (the kernels' `anchor`) and
  complex products, eight harmonics at a time, give every sin(h·Φ); with the fixed data, slot
  i is harmonic i + 1 and the sines are read as one slice. Plain Rust, vectorized by the
  compiler; nothing allocated on the audio thread (the table is 512 floats in the voice).
  Deviation from the trio (`tests/fast.rs`, 7 sirens at 44.1 and 48 kHz, 4 s each): at most
  6.1e-6 of the peak, an SNR of at least 116.3 dB; the test bounds are 3e-5 and 100 dB. The
  harmonic sines alone are within 2.2e-4 of libm's up to h = 511 (bound 5e-4); the low
  harmonics, which carry the energy, are much closer.
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

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| S1 | 137 (152x) | 124 (168x) | 123 (170x) | 128 (163x) | 990 (21x) | 990 (21x) | 992 (21x) | 431 (48x) | 430 (48x) | 431 (48x) | 341 (61x) | 343 (61x) | 342 (61x) |
| S2 | 137 (152x) | 124 (168x) | 124 (169x) | 127 (164x) | 991 (21x) | 987 (21x) | 991 (21x) | 430 (48x) | 429 (49x) | 431 (48x) | 342 (61x) | 343 (61x) | 343 (61x) |
| S3 | 209 (100x) | 197 (106x) | 198 (105x) | 203 (103x) | 1056 (20x) | 1052 (20x) | 1056 (20x) | 446 (47x) | 446 (47x) | 447 (47x) | 328 (64x) | 322 (65x) | 328 (64x) |
| S4 | 228 (91x) | 218 (95x) | 218 (96x) | 223 (94x) | 1087 (19x) | 1083 (19x) | 1086 (19x) | 446 (47x) | 446 (47x) | 448 (46x) | 358 (58x) | 358 (58x) | 358 (58x) |
| S5 | 286 (73x) | 281 (74x) | 280 (74x) | 286 (73x) | 1176 (18x) | 1171 (18x) | 1177 (18x) | 455 (46x) | 453 (46x) | 452 (46x) | 360 (58x) | 358 (58x) | 358 (58x) |
| S6 | 287 (73x) | 277 (75x) | 280 (74x) | 286 (73x) | 1178 (18x) | 1169 (18x) | 1176 (18x) | 455 (46x) | 454 (46x) | 454 (46x) | 360 (58x) | 359 (58x) | 357 (58x) |
| S7 | 253 (82x) | 244 (86x) | 246 (85x) | 252 (83x) | 1288 (16x) | 1278 (16x) | 1288 (16x) | 449 (46x) | 449 (46x) | 448 (46x) | 404 (52x) | 404 (52x) | 403 (52x) |

### Sample rates (S3, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 44.1 kHz | 210 (108x) | 199 (114x) | 198 (115x) | 203 (111x) | 1052 (22x) | 1051 (22x) | 1052 (22x) | 444 (51x) | 442 (51x) | 444 (51x) | 324 (70x) | 318 (71x) | 322 (70x) |
| 48 kHz | 209 (100x) | 199 (105x) | 199 (105x) | 204 (102x) | 1056 (20x) | 1051 (20x) | 1055 (20x) | 446 (47x) | 445 (47x) | 446 (47x) | 329 (63x) | 324 (64x) | 329 (63x) |
| 96 kHz | 205 (51x) | 193 (54x) | 190 (55x) | 194 (54x) | 1044 (10x) | 1041 (10x) | 1046 (10x) | 445 (23x) | 443 (24x) | 445 (23x) | 327 (32x) | 322 (32x) | 328 (32x) |

### Quality (S3, 48 kHz, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 30 partials | 210 (99x) | 198 (105x) | 198 (105x) | 203 (102x) | 1056 (20x) | 1051 (20x) | 1055 (20x) | 446 (47x) | 445 (47x) | 447 (47x) | 329 (63x) | 324 (64x) | 329 (63x) |
| 120 partials | 877 (24x) | 803 (26x) | 799 (26x) | 812 (26x) | 4163 (5x) | 4149 (5x) | 4165 (5x) | 1402 (15x) | 1402 (15x) | 1404 (15x) | 1133 (18x) | 1133 (18x) | 1134 (18x) |
| 200 partials | 1367 (15x) | 1224 (17x) | 1237 (17x) | 1294 (16x) | 7453 (3x) | 7415 (3x) | 7445 (3x) | 2257 (9x) | 2258 (9x) | 2260 (9x) | 1780 (12x) | 1778 (12x) | 1777 (12x) |

### Block size (S3, 48 kHz, 30 partials)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 samples | 209 (100x) | 196 (106x) | 198 (105x) | 204 (102x) | 1055 (20x) | 1052 (20x) | 1055 (20x) | 445 (47x) | 446 (47x) | 447 (47x) | 329 (63x) | 325 (64x) | 330 (63x) |
| 128 samples | 209 (100x) | 197 (106x) | 199 (105x) | 204 (102x) | 1056 (20x) | 1051 (20x) | 1056 (20x) | 445 (47x) | 444 (47x) | 446 (47x) | 328 (64x) | 325 (64x) | 331 (63x) |
| 256 samples | 209 (100x) | 197 (106x) | 197 (106x) | 203 (102x) | 1056 (20x) | 1052 (20x) | 1056 (20x) | 447 (47x) | 446 (47x) | 447 (47x) | 330 (63x) | 324 (64x) | 329 (63x) |
| 512 samples | 208 (100x) | 198 (105x) | 197 (106x) | 204 (102x) | 1056 (20x) | 1051 (20x) | 1057 (20x) | 445 (47x) | 445 (47x) | 445 (47x) | 327 (64x) | 323 (64x) | 329 (63x) |

## MacBook Air M4 (aarch64)

The same tables on a MacBook Air, Apple M4, macOS 15.3, rustc 1.94.1, Release, from a fresh
clone at 3b4b308. The crate tests (`tests/fast.rs` included, with the same measured
deviations), the C++ Release builds with `COMPOSESIREN_RUST_DSP` OFF and ON and their ctests
(`rust-dsp-parity` included) pass there too. Everything is 2.3 to 3.4× faster than on the
Mac mini; the fast trio costs 1.6× the C++ there, and the f32 trio about the C++'s cost at
30 partials (95 against 89 ns per sample) and less than it at 200 (621 against 654).

#### Each siren (48 kHz, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| S1 | 56 (374x) | 48 (437x) | 47 (439x) | 47 (439x) | 263 (79x) | 261 (80x) | 263 (79x) | 138 (151x) | 138 (151x) | 138 (151x) | 96 (217x) | 96 (216x) | 93 (225x) |
| S2 | 56 (373x) | 48 (438x) | 48 (436x) | 47 (441x) | 263 (79x) | 263 (79x) | 263 (79x) | 138 (151x) | 138 (151x) | 138 (151x) | 92 (227x) | 91 (228x) | 92 (228x) |
| S3 | 89 (235x) | 78 (267x) | 78 (266x) | 76 (274x) | 305 (68x) | 300 (69x) | 306 (68x) | 139 (150x) | 139 (149x) | 139 (150x) | 95 (220x) | 94 (221x) | 94 (221x) |
| S4 | 101 (207x) | 89 (234x) | 91 (230x) | 84 (247x) | 338 (62x) | 332 (63x) | 336 (62x) | 139 (150x) | 139 (150x) | 141 (148x) | 98 (212x) | 99 (211x) | 98 (212x) |
| S5 | 126 (165x) | 112 (186x) | 113 (184x) | 104 (200x) | 373 (56x) | 371 (56x) | 369 (57x) | 141 (148x) | 141 (148x) | 142 (147x) | 101 (207x) | 101 (206x) | 101 (206x) |
| S6 | 126 (165x) | 112 (187x) | 112 (185x) | 103 (201x) | 388 (54x) | 386 (54x) | 388 (54x) | 140 (149x) | 141 (148x) | 140 (149x) | 101 (206x) | 110 (190x) | 110 (190x) |
| S7 | 122 (170x) | 106 (197x) | 106 (197x) | 105 (198x) | 466 (45x) | 462 (45x) | 467 (45x) | 140 (148x) | 140 (148x) | 140 (148x) | 116 (180x) | 115 (181x) | 115 (180x) |

#### Sample rates (S3, 30 partials, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 44.1 kHz | 89 (253x) | 78 (290x) | 78 (291x) | 76 (297x) | 316 (72x) | 313 (72x) | 319 (71x) | 139 (163x) | 140 (163x) | 139 (163x) | 99 (228x) | 97 (234x) | 96 (236x) |
| 48 kHz | 89 (235x) | 78 (267x) | 78 (266x) | 76 (274x) | 318 (66x) | 310 (67x) | 308 (68x) | 140 (149x) | 139 (150x) | 139 (150x) | 95 (220x) | 94 (221x) | 95 (219x) |
| 96 kHz | 86 (121x) | 76 (137x) | 74 (140x) | 73 (142x) | 305 (34x) | 303 (34x) | 306 (34x) | 139 (75x) | 140 (75x) | 140 (75x) | 94 (111x) | 94 (111x) | 95 (110x) |

#### Quality (S3, 48 kHz, 256-sample blocks)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 30 partials | 89 (234x) | 78 (267x) | 79 (265x) | 76 (273x) | 315 (66x) | 301 (69x) | 306 (68x) | 140 (149x) | 140 (149x) | 140 (148x) | 95 (219x) | 95 (220x) | 95 (219x) |
| 120 partials | 420 (50x) | 359 (58x) | 359 (58x) | 357 (58x) | 1366 (15x) | 1350 (15x) | 1360 (15x) | 474 (44x) | 473 (44x) | 476 (44x) | 370 (56x) | 369 (56x) | 370 (56x) |
| 200 partials | 654 (32x) | 549 (38x) | 553 (38x) | 590 (35x) | 2491 (8x) | 2471 (8x) | 2504 (8x) | 760 (27x) | 760 (27x) | 759 (27x) | 621 (34x) | 627 (33x) | 625 (33x) |

#### Block size (S3, 48 kHz, 30 partials)

ns per sample (real-time factor: voices one core could play), median of 7 runs of 2 s.

| case | cpp | rust-exact | rust-time | frames | trio | trio-linear | trio-nearest | trio-fast | trio-linear-fast | trio-nearest-fast | trio-f32 | trio-linear-f32 | trio-nearest-f32 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 samples | 89 (234x) | 79 (263x) | 79 (264x) | 77 (271x) | 306 (68x) | 302 (69x) | 306 (68x) | 139 (150x) | 139 (149x) | 139 (150x) | 95 (220x) | 94 (221x) | 95 (219x) |
| 128 samples | 90 (232x) | 79 (265x) | 79 (265x) | 77 (272x) | 309 (67x) | 302 (69x) | 306 (68x) | 139 (150x) | 139 (150x) | 139 (150x) | 96 (218x) | 95 (219x) | 95 (220x) |
| 256 samples | 89 (234x) | 79 (265x) | 79 (263x) | 76 (274x) | 313 (67x) | 311 (67x) | 316 (66x) | 140 (149x) | 140 (149x) | 140 (149x) | 95 (220x) | 95 (220x) | 95 (218x) |
| 512 samples | 89 (234x) | 78 (266x) | 79 (264x) | 76 (272x) | 309 (68x) | 300 (69x) | 310 (67x) | 140 (149x) | 140 (149x) | 139 (150x) | 95 (220x) | 94 (221x) | 94 (222x) |

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
