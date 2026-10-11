// The compact format against the raw files: every data set of Resources/ packs and
// unpacks back to the same bytes, a damaged file is refused, and nothing is lost.
//
//   dotnet fsi tests/compact-roundtrip.fsx [<raw dir>]     (default: ../Resources)
// Exits 1 on any failure.
#load "../SirenData.fs"

open System
open System.IO

let dir =
    match fsi.CommandLineArgs |> Array.skip 1 with
    | [| d |] -> d
    | _ -> Path.Combine(__SOURCE_DIRECTORY__, "..", "..", "Resources")

let mutable failures = 0
let check ok what =
    if not ok then
        failures <- failures + 1
        eprintfn "FAIL: %s" what

for name in SirenData.dataSets do
    let raw = SirenData.readRaw dir name
    let packed = SirenData.pack raw
    let back = SirenData.unpack name packed
    check (back.Amp.AsSpan().SequenceEqual(raw.Amp)) $"{name}: dataAmp round trip"
    check (back.Freq.AsSpan().SequenceEqual(raw.Freq)) $"{name}: dataFreq round trip"
    check (back.Dure.AsSpan().SequenceEqual(raw.Dure)) $"{name}: datadureTabs round trip"
    // determinism: packing twice gives the same bytes
    check ((SirenData.pack raw).AsSpan().SequenceEqual(packed)) $"{name}: pack is deterministic"
    // a flipped byte in the body is caught by the CRC
    let damaged = Array.copy packed
    damaged[damaged.Length / 2] <- damaged[damaged.Length / 2] ^^^ 0x01uy
    let refused = try SirenData.unpack name damaged |> ignore; false with _ -> true
    check refused $"{name}: a damaged file is refused"
    let rawMB = float (raw.Amp.Length + raw.Freq.Length + raw.Dure.Length) / 1e6
    printfn "%s: %d notes, %.1f MB raw, %.1f MB compact, round trip ok" name raw.FileNotes rawMB (float packed.Length / 1e6)

if failures > 0 then
    eprintfn "%d failure(s)" failures
    exit 1
printfn "compact round trip: all data sets byte-identical"
