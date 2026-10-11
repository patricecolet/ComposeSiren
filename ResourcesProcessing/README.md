# ComposeSiren's Resources directory

The root-level `Resources` directory contains raw spectral data extracted from
no longer available historical audio recordings of all the different sirens' notes.
Not only were these recordings lost, there is also no documentation of the process
used to extract this spectral data.

The files in this directory represent the actual ground truth for the sirens' audio
resynthesis. Each siren type (N) is modeled by 4 data files :
* `datadureTabsSN`
* `dataFreqSN`
* `dataAmpSN`
* `dataVectorIntervalSN`

Each one of these 4 files contains data related to a particular siren model and
comes as a multidimensional array of floating point numbers. Together, for each
note, they provide a succession of spectral snapshots (frames) containing
information about what seems to be the most salient frequencies.
* `datadureTabsSN[n]` contains the array `[ fdur, fmax, meanFreq ]` where `fdur`
is how long in samples (at sample rate `sr`) each frame, or spectral snapshot,
of the note `n` must be played. `fmax` represents the number of frames (snapshots)
to loop on, and `meanFreq` is the mean fundamental frequency of the note in Hz.
* `dataFreqSN[n][f]` contains a zero-tabbed array of all frequancies in Hz for
the note `n` at frame `f`.
* For each frequency in `dataFreqSN[n][f]`, `dataAmpSN[n][f]` contains its
amplitude.
* `dataVectorIntervalSN` contains note-specific inertia information.

Both the plugins' audio engine and the data files can be modified in attempts to
improve the quality and realism of the audio resynthesis.

Ultimately, new records should be made and more recent ML approaches should be
used to model the sirens' audio.




# The crossfade fix (`fix_data.fsx`)

With `PROCESS_RESOURCES=ON` (the default) the build does not ship `Resources/` itself but
a fixed copy, written to `<build>/Resources-processed` by `fix_data.fsx` (`FixData.fs`):

1. the partials are re-indexed by physical harmonic number, slot `h` holding harmonic `h+1`
   of the data set's grid fundamental (on a collision the stronger one is kept);
2. the frequencies are set to exact harmonics `f0*(h+1)`, keeping the amplitudes.

Every step between adjacent notes is then exactly a semitone for every harmonic, so the
plugin's crossfade between notes no longer produces ghost frequencies. The grid is detected per
data set by an amplitude-weighted vote (S7 sits an octave above the others). `Resources/` is
only read.

```sh
dotnet fsi ResourcesProcessing/fix_data.fsx [--resources <dir>] [--output <dir>] [--dry-run]
dotnet fsi ResourcesProcessing/tests/fix-data.fsx    # output vs tests/fix-data-expected.sha256
```

The F# replaced an earlier numpy script; `tests/fix-data-expected.sha256` is that script's
output on the current `Resources/`, which the F# reproduces byte for byte (about 2 s for all
data sets).

# The compact format

[compact-format.md](compact-format.md): an optional, lossless single-file packing of each data
set (603 MB → 57 MB), unpacked back to the raw files byte for byte. F# tools (.NET SDK):

```sh
dotnet fsi ResourcesProcessing/compact.fsx pack Resources <out dir>
dotnet fsi ResourcesProcessing/tests/compact-roundtrip.fsx
```
