// Pack siren data sets into the compact format, or unpack them back to the raw files.
//
//   dotnet fsi compact.fsx pack   <raw dir> <out dir>   # dataAmp/dataFreq/datadureTabsSN -> SN.sirendata
//   dotnet fsi compact.fsx unpack <compact dir> <out dir>
//
// The dataVectorInterval* files are small and copied as they are. Format: compact-format.md.
#load "SirenData.fs"

open System
open System.Diagnostics
open System.IO

let copyVectorIntervals (src: string) (dst: string) =
    for f in Directory.GetFiles(src, "dataVectorInterval*") do
        File.Copy(f, Path.Combine(dst, Path.GetFileName f), true)

let pack (src: string) (dst: string) =
    Directory.CreateDirectory dst |> ignore
    let mutable rawTotal, compactTotal = 0L, 0L
    for name in SirenData.dataSets do
        let sw = Stopwatch.StartNew()
        let raw = SirenData.readRaw src name
        let bytes = SirenData.pack raw
        File.WriteAllBytes(Path.Combine(dst, SirenData.compactFileName name), bytes)
        let rawBytes = int64 (raw.Amp.Length + raw.Freq.Length + raw.Dure.Length)
        rawTotal <- rawTotal + rawBytes
        compactTotal <- compactTotal + int64 bytes.Length
        printfn "%s: %6.1f MB -> %5.1f MB (%d ms)" name (float rawBytes / 1e6) (float bytes.Length / 1e6) sw.ElapsedMilliseconds
    copyVectorIntervals src dst
    printfn "total: %.1f MB -> %.1f MB" (float rawTotal / 1e6) (float compactTotal / 1e6)

let unpack (src: string) (dst: string) =
    for name in SirenData.dataSets do
        let path = Path.Combine(src, SirenData.compactFileName name)
        SirenData.unpack name (File.ReadAllBytes path) |> SirenData.writeRaw dst
        printfn "%s: unpacked" name
    copyVectorIntervals src dst

match fsi.CommandLineArgs |> Array.skip 1 with
| [| "pack"; src; dst |] -> pack src dst
| [| "unpack"; src; dst |] -> unpack src dst
| _ ->
    eprintfn "usage: dotnet fsi compact.fsx (pack|unpack) <from dir> <to dir>"
    exit 2
