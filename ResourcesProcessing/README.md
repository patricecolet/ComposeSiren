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




# The compact format

[compact-format.md](compact-format.md): an optional, lossless single-file packing of each data
set (603 MB → 57 MB), unpacked back to the raw files byte for byte. F# tools (.NET SDK):

```sh
dotnet fsi ResourcesProcessing/compact.fsx pack Resources <out dir>
dotnet fsi ResourcesProcessing/tests/compact-roundtrip.fsx
```
