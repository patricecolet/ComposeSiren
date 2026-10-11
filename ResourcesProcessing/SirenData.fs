/// The siren data files, in the two formats ComposeSiren can read.
///
/// Raw (`Resources/`, the ground truth): per data set N (S1, S3, S4, S5, S7)
/// - `dataAmpSN`, `dataFreqSN`: little-endian float32 `[note][1000][200]`, for as many
///   notes as the file holds (72, 73 or 80); the plugin reads them into `[80][1000][200]`
///   tables, zero past the file's end
/// - `datadureTabsSN`: float32 `[note][3]` = samples per frame, frames looped, mean frequency
/// - `dataVectorIntervalSN`: 392 float32 rotor inertia rates (copied as they are)
///
/// Compact (`<N>.sirendata`, see compact-format.md): one file per data set holding
/// `dataAmp`, `dataFreq` and `datadureTabs`, each note cut after its last frame with any
/// non-zero byte in amp or freq. Unpacking gives the raw files back byte for byte.
module SirenData

open System
open System.IO
open System.Runtime.InteropServices

[<Literal>]
let Frames = 1000

[<Literal>]
let Slots = 200

[<Literal>]
let FrameFloats = 200 // Slots, a float32 per slot

/// The data sets ComposeSiren ships (S2 plays S1's, S6 plays S5's).
let dataSets = [| "S1"; "S3"; "S4"; "S5"; "S7" |]

/// The raw files of one data set, as bytes.
type Raw =
    { Name : string
      Amp  : byte[]
      Freq : byte[]
      Dure : byte[] }

    member r.FileNotes = r.Amp.Length / (Frames * Slots * 4)

let readRaw (dir: string) (name: string) =
    let read prefix = File.ReadAllBytes(Path.Combine(dir, prefix + name))
    let r = { Name = name; Amp = read "dataAmp"; Freq = read "dataFreq"; Dure = read "datadureTabs" }
    let note = Frames * Slots * 4
    if r.Amp.Length % note <> 0 || r.Freq.Length <> r.Amp.Length then
        failwith $"{dir}: dataAmp{name} / dataFreq{name} are not whole [note][{Frames}][{Slots}] float32 tables"
    if r.Dure.Length % 4 <> 0 then
        failwith $"{dir}: datadureTabs{name} is not a whole number of float32"
    r

let writeRaw (dir: string) (r: Raw) =
    Directory.CreateDirectory dir |> ignore
    File.WriteAllBytes(Path.Combine(dir, "dataAmp" + r.Name), r.Amp)
    File.WriteAllBytes(Path.Combine(dir, "dataFreq" + r.Name), r.Freq)
    File.WriteAllBytes(Path.Combine(dir, "datadureTabs" + r.Name), r.Dure)

// --- CRC-32 (IEEE 802.3, the zlib one), so the Rust reader needs no dependency ---

let private crcTable =
    Array.init 256 (fun n ->
        let mutable c = uint32 n
        for _ in 0 .. 7 do
            c <- if c &&& 1u <> 0u then 0xEDB88320u ^^^ (c >>> 1) else c >>> 1
        c)

let crc32 (bytes: ReadOnlySpan<byte>) =
    let mutable c = 0xFFFFFFFFu
    for b in bytes do
        c <- crcTable[int ((c ^^^ uint32 b) &&& 0xFFu)] ^^^ (c >>> 8)
    c ^^^ 0xFFFFFFFFu

// --- compact-v1 ---

let magic = "CSSD"B
[<Literal>]
let Version = 1u

/// Frames kept for each note: one past the last frame with any non-zero byte in amp or
/// freq (bytes, so -0.0 counts as data and round-trips).
let keptFrames (r: Raw) =
    let frameBytes = FrameFloats * 4
    let noteBytes = Frames * frameBytes
    Array.init r.FileNotes (fun n ->
        let mutable last = 0
        for f in 0 .. Frames - 1 do
            let o = n * noteBytes + f * frameBytes
            let a = ReadOnlySpan(r.Amp, o, frameBytes)
            let q = ReadOnlySpan(r.Freq, o, frameBytes)
            if a.IndexOfAnyExcept(0uy) >= 0 || q.IndexOfAnyExcept(0uy) >= 0 then last <- f + 1
        last)

/// The compact file of a data set.
let pack (r: Raw) : byte[] =
    let kept = keptFrames r
    use ms = new MemoryStream()
    use w = new BinaryWriter(ms)
    w.Write magic
    w.Write Version
    w.Write(uint32 Slots)
    w.Write(uint32 Frames)
    w.Write(uint32 r.FileNotes)
    w.Write(uint32 (r.Dure.Length / 4))
    w.Write r.Dure
    for k in kept do w.Write(uint32 k)
    let frameBytes = FrameFloats * 4
    let noteBytes = Frames * frameBytes
    for n in 0 .. r.FileNotes - 1 do
        w.Write(r.Amp, n * noteBytes, kept[n] * frameBytes)
        w.Write(r.Freq, n * noteBytes, kept[n] * frameBytes)
    w.Flush()
    let body = ms.ToArray()
    w.Write(crc32 (ReadOnlySpan body))
    w.Flush()
    ms.ToArray()

/// The raw files back from a compact file, byte for byte.
let unpack (name: string) (bytes: byte[]) : Raw =
    if bytes.Length < 28 || not (ReadOnlySpan(bytes, 0, 4).SequenceEqual(ReadOnlySpan magic)) then
        failwith $"{name}: not a compact siren data file (magic)"
    let stored = BitConverter.ToUInt32(bytes, bytes.Length - 4)
    if crc32 (ReadOnlySpan(bytes, 0, bytes.Length - 4)) <> stored then
        failwith $"{name}: CRC mismatch, the file is damaged"
    use r = new BinaryReader(new MemoryStream(bytes, 0, bytes.Length - 4))
    r.ReadBytes 4 |> ignore
    let version = r.ReadUInt32()
    if version <> Version then failwith $"{name}: compact format version {version}, this reader knows {Version}"
    let slots = int (r.ReadUInt32())
    let frames = int (r.ReadUInt32())
    if slots <> Slots || frames <> Frames then failwith $"{name}: {frames} x {slots} tables, expected {Frames} x {Slots}"
    let notes = int (r.ReadUInt32())
    let dureFloats = int (r.ReadUInt32())
    let dure = r.ReadBytes(dureFloats * 4)
    let kept = Array.init notes (fun _ -> int (r.ReadUInt32()))
    let frameBytes = FrameFloats * 4
    let noteBytes = Frames * frameBytes
    let amp = Array.zeroCreate<byte> (notes * noteBytes)
    let freq = Array.zeroCreate<byte> (notes * noteBytes)
    for n in 0 .. notes - 1 do
        let len = kept[n] * frameBytes
        if r.Read(amp, n * noteBytes, len) <> len || r.Read(freq, n * noteBytes, len) <> len then
            failwith $"{name}: truncated at note {n}"
    { Name = name; Amp = amp; Freq = freq; Dure = dure }

let compactFileName (name: string) = name + ".sirendata"
