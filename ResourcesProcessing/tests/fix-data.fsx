// FixData.fs against the recorded output of the original numpy implementation:
// fix Resources/ into a temporary directory and compare every file's SHA-256 with
// tests/fix-data-expected.sha256. Then the fixed data sets go through the compact format
// and come back byte for byte.
//
//   dotnet fsi tests/fix-data.fsx [<raw dir>]     (default: ../Resources). Exits 1 on failure.
#load "../FixData.fs"
#load "../SirenData.fs"

open System
open System.IO
open System.Security.Cryptography

let resources =
    match fsi.CommandLineArgs |> Array.skip 1 with
    | [| d |] -> d
    | _ -> Path.Combine(__SOURCE_DIRECTORY__, "..", "..", "Resources")

let out = Path.Combine(Path.GetTempPath(), $"composesiren-fix-data-{Guid.NewGuid():N}")
let mutable failures = 0
try
    FixData.run resources (Some out)
    let expected =
        File.ReadAllLines(Path.Combine(__SOURCE_DIRECTORY__, "fix-data-expected.sha256"))
        |> Array.filter (fun l -> l.Trim() <> "" && not (l.StartsWith "#"))
        |> Array.map (fun l -> let p = l.Split("  ") in p[1].Trim(), p[0].Trim())
    for name, hash in expected do
        let path = Path.Combine(out, name)
        let actual =
            if File.Exists path then Convert.ToHexString(SHA256.HashData(File.ReadAllBytes path)).ToLowerInvariant()
            else "missing"
        if actual <> hash then
            failures <- failures + 1
            eprintfn "FAIL: %s: %s, expected %s" name actual hash
    let extra = Directory.GetFiles out |> Array.map Path.GetFileName |> Array.except (Array.map fst expected)
    for e in extra do
        failures <- failures + 1
        eprintfn "FAIL: unexpected output file %s" e
    printfn "fix_data: %d files checked against the reference" expected.Length
    for name in SirenData.dataSets do
        let raw = SirenData.readRaw out name
        let back = SirenData.unpack name (SirenData.pack raw)
        if not (back.Amp.AsSpan().SequenceEqual(raw.Amp) && back.Freq.AsSpan().SequenceEqual(raw.Freq) && back.Dure.AsSpan().SequenceEqual(raw.Dure)) then
            failures <- failures + 1
            eprintfn "FAIL: %s: fixed data compact round trip" name
    printfn "fixed data: compact round trip checked"
finally
    if Directory.Exists out then Directory.Delete(out, true)

if failures > 0 then
    eprintfn "%d failure(s)" failures
    exit 1
printfn "fix_data: byte-identical with the reference"
