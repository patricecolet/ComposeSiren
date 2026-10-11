# The compact siren data format (`.sirendata`, version 1)

An optional, lossless packing of a data set's three big files (`dataAmpSN`, `dataFreqSN`,
`datadureTabsSN`, see [README.md](README.md)). The raw files hold `[note][1000][200]` float32
tables, mostly zeros: each note uses a few dozen to a few hundred frames. The compact file
keeps, for each note, the frames up to its last one with any non-zero byte, and nothing else.

Unpacking gives the raw files back **byte for byte**, so anything that reads the raw files
reads the same values from a compact file, including every read past a note's data (zeros).
With the default build options ComposeSiren ships and reads the raw files; this format is an
option.

| data set | raw (amp + freq + dure) | compact |
|---|---|---|
| S1 | 116.8 MB | 9.3 MB |
| S3 | 115.2 MB | 8.3 MB |
| S4 | 115.2 MB | 18.8 MB |
| S5 | 128.0 MB | 16.0 MB |
| S7 | 128.0 MB | 4.5 MB |
| total | 603 MB | 57 MB |

`dataVectorIntervalSN` (1568 bytes each) stays a separate file, as it is.

## Layout

Little-endian throughout. `u32` is an unsigned 32-bit integer, `f32` an IEEE-754 float.

| field | type | value |
|---|---|---|
| magic | 4 bytes | `CSSD` |
| version | u32 | 1 |
| slots | u32 | 200 (partial slots per frame) |
| frames | u32 | 1000 (frames per note in the raw tables) |
| notes | u32 | notes in the raw tables (72, 73 or 80) |
| dure count | u32 | number of f32 in `datadureTabsSN` |
| dure | f32 × dure count | `datadureTabsSN`, verbatim (`[note][3]`: fdur, fmax, meanFreq) |
| kept | u32 × notes | frames stored for each note |
| body | | for each note: `amp[kept][200]` f32, then `freq[kept][200]` f32 |
| crc | u32 | CRC-32 (IEEE, as zlib) of every byte before it |

A frame is kept up to the last one where **any byte** of amp or freq is non-zero, so `-0.0`
is data. A reader rebuilds a raw note as its kept frames followed by zeros up to 1000 frames.

## Tools

- `SirenData.fs`: read and write the raw files and the compact format (F#).
- `compact.fsx`: `dotnet fsi compact.fsx pack <raw dir> <out dir>` writes `SN.sirendata` for
  S1, S3, S4, S5 and S7 and copies the `dataVectorInterval*` files; `unpack` goes back.
- `tests/compact-roundtrip.fsx`: packs and unpacks every data set of `Resources/` and checks
  the bytes, the determinism of `pack`, and that a damaged file is refused.
