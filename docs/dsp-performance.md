# DSP performance: the Rust voices against the C++ voice

Measured 2026-10-12 on one machine: Mac mini 2018, Intel Core i5-8500B 3.0 GHz, macOS 15.7.9,
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
- `rust-exact`, which plays the same samples as the C++ bit for bit, costs about 1.6× the C++
  (S3 at 48 kHz: 352 against 217 ns per sample). That's still about 59 voices per core at 30
  partials and 9 at 200. `rust-time` and `frames` cost the same as `rust-exact`.
- The trio variants cost about 8× the C++ (about 12 voices per core at 30 partials).
- Block size, from 64 to 512 samples, changes nothing; time per sample doesn't depend on the
  sample rate, so the real-time factor halves at 96 kHz.
- Loading: the C++ loader takes 6.7 s and 854 MiB for every siren's data. The Rust raw loader
  takes 0.44 s and 299 MiB, and the compact `.sirendata` files take 0.17 s and 94 MiB.

Making the Rust voices faster is left for later work. The lab's SIMD kernels are a lead.

## Load time and memory

Load every siren's data once and measure the growth in resident memory (median of 3 runs).

| loader | data | time | memory |
|---|---|---:|---:|
| C++ (`cpp`) | raw `Resources/` | 6.7 s | 854 MiB |
| Rust `siren-data` | raw `Resources/` | 0.44 s | 299 MiB |
| Rust `siren-data` | compact `.sirendata` (57 MB on disk) | 0.17 s | 94 MiB |

## Time per sample
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
