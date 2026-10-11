/// The crossfade artifact fix for the siren data files (formerly fix_data.py).
///
/// Non-destructive: the files of Resources/ (the ground truth) are only read; the fixed files
/// go to another directory, in the same raw format (drop-in).
///
/// The fix, per data set:
///   1. Re-index the partials by physical harmonic number: slot h holds harmonic h+1 of the
///      grid fundamental. On a collision the stronger harmonic is kept.
///   2. Force the frequencies to exact harmonics f0*(h+1), keeping the amplitudes. Every step
///      between adjacent notes is then exactly a semitone for every harmonic, so the plugin's
///      crossfade no longer makes ghost frequencies.
/// The harmonic grid is detected per data set by an amplitude-weighted vote: the code's nominal
/// pitch is 440*2^((note-69)/12); S1, S3, S4 and S5 sit on that grid, S7 an octave above.
///
/// Byte-exact with the numpy original: the 2-D frame means are a sequential f32 accumulator,
/// the evolving collision slot is numpy's pairwise summation (see npySum), thresholds are f32.
module FixData

open System
open System.Collections.Generic
open System.Diagnostics
open System.IO
open System.Runtime.InteropServices

[<Literal>]
let MaxTab = 1000

[<Literal>]
let MaxPartiel = 200

/// below this a slot counts as empty (python: SEUIL_AMP), f32 to match
/// numpy's NEP-50 cast of the python literal to f32
let seuilAmp = 1e-7f
let seuilPeak = 1e-4f
let seuilVote = 0.15
let sirenes = [ "S1"; "S3"; "S4"; "S5"; "S7" ]

/// f0 of a table index: the S7 correction's nominal fundamental.
let f0Theorique (note: int) = 440.0 * 2.0 ** (float (note - 57) / 12.0)

// --- raw f32 file IO ----------------------------------------------------------------

let readF32 (path: string) =
    let bytes = File.ReadAllBytes path
    if bytes.Length % 4 <> 0 then
        failwithf "%s: %d bytes, not a multiple of 4" path bytes.Length
    let v = Array.zeroCreate<float32> (bytes.Length / 4)
    bytes.AsSpan().CopyTo(MemoryMarshal.AsBytes(v.AsSpan()))
    v

let writeF32 (path: string) (v: float32[]) =
    File.WriteAllBytes(path, MemoryMarshal.AsBytes(v.AsSpan()).ToArray())

// --- frame statistics ------------------------------------------------------------------

/// numpy's 2-D axis-0 mean for one slot: sequential f32 accumulation and one
/// f32 division (verified: 0 mismatches / 88,000 slots against numpy 2.5.3).
let frameStats (tab: float32[]) base' nb =
    let acc = Array.zeroCreate<float32> MaxPartiel
    for i in 0 .. nb - 1 do
        let row = base' + i * MaxPartiel
        for h in 0 .. MaxPartiel - 1 do
            acc[h] <- acc[h] + tab[row + h]
    let inv = float32 nb
    for h in 0 .. MaxPartiel - 1 do
        acc[h] <- acc[h] / inv
    acc

/// numpy's pairwise summation, bit for bit (loops_utils.h.src: n < 8
/// sequential from -0.0; n <= 128 eight accumulators with an 8-aligned tail;
/// else split at n/2 - (n/2)%8 and recurse).
let rec npySum (n: int) (get: int -> float32) : float32 =
    if n < 8 then
        let mutable res = -0.0f
        for i in 0 .. n - 1 do
            res <- res + get i
        res
    elif n <= 128 then
        let r = Array.zeroCreate<float32> 8
        for j in 0 .. 7 do
            r[j] <- get j
        let mutable i = 8
        let stop = n - n % 8
        while i < stop do
            for j in 0 .. 7 do
                r[j] <- r[j] + get (i + j)
            i <- i + 8
        let mutable res = ((r[0] + r[1]) + (r[2] + r[3])) + ((r[4] + r[5]) + (r[6] + r[7]))
        while i < n do
            res <- res + get i
            i <- i + 1
        res
    else
        let half = n / 2
        let n2 = half - half % 8
        npySum n2 get + npySum (n - n2) (fun i -> get (n2 + i))

/// new_amp[note, :nb, cible].mean() — numpy pairwise for the 1-D evolving
/// slice (verified: 0 mismatches / 25,639 slices against numpy 2.5.3).
let slotMean (tab: float32[]) base' h nb =
    npySum nb (fun i -> tab[base' + i * MaxPartiel + h]) / float32 nb

let maxFrames (tab: float32[]) base' h nb =
    let mutable m = Single.NegativeInfinity
    for i in 0 .. nb - 1 do
        let x = tab[base' + i * MaxPartiel + h]
        if x > m then m <- x
    m

// --- detecter_grille -------------------------------------------------------------------

/// Amplitude-weighted vote: for each usable note, the active slots are tested
/// against f0·g for g ∈ {1.0, 0.5, 0.25} (first pass wins the note), and each
/// win adds the note's total active amplitude to votes[g]. Returns 1.0 (S7) or
/// 0.5 (S1/S3/S4/S5).
let detecterGrille (freq: float32[]) (amp: float32[]) (dure: float32[]) nNotes =
    let votes = Dictionary<float, float>()
    for note in 0 .. nNotes - 1 do
        let nb = int dure[note * 3 + 1]
        if nb > 0 then
            let base' = note * MaxTab * MaxPartiel
            let avgF = frameStats freq base' nb
            let peak = Array.create MaxPartiel Single.NegativeInfinity
            for i in 0 .. nb - 1 do
                let row = base' + i * MaxPartiel
                for h in 0 .. MaxPartiel - 1 do
                    let x = amp[row + h]
                    if x > peak[h] then peak[h] <- x
            let active =
                [ for h in 0 .. MaxPartiel - 1 do
                    if peak[h] > seuilPeak && avgF[h] > 20.0f then
                        yield float avgF[h], float peak[h] ]
            if active.Length >= 4 then
                let f0 = f0Theorique note
                let mutable won = false
                for g in [ 1.0; 0.5; 0.25 ] do
                    if not won then
                        let denom = f0 * g
                        let mutable wErr = 0.0
                        let mutable wSum = 0.0
                        let mutable aSum = 0.0
                        for f, a in active do
                            let nH = f / denom
                            let err = abs (nH - Math.Round(nH, MidpointRounding.ToEven))
                            wErr <- wErr + err * a
                            wSum <- wSum + a
                            aSum <- aSum + a
                        if wErr / wSum < seuilVote then
                            won <- true
                            match votes.TryGetValue g with
                            | true, v -> votes[g] <- v + aSum
                            | _ -> votes[g] <- aSum
    if votes.Count = 0 then
        failwith "aucune note exploitable pour détecter la grille"
    let mutable bestG = 0.0
    let mutable bestVotes = -1.0
    for kv in votes do
        if kv.Value > bestVotes then
            bestVotes <- kv.Value
            bestG <- kv.Key
    bestG

// --- corriger_sirene -------------------------------------------------------------------

type Stats =
    { Collisions: int
      Perdues: int
      PerduesAudibles: int
      HarmMax: int }

let corrigerSirene (freq: float32[]) (amp: float32[]) (dure: float32[]) nNotes g =
    let total = nNotes * MaxTab * MaxPartiel
    let newAmp = Array.zeroCreate<float32> total
    let newFreq = Array.zeroCreate<float32> total
    // per-note counters (notes are independent: each writes only its own
    // [nbase .. nbase+MaxTab*MaxPartiel) slice of newAmp/newFreq/dure)
    let coll = Array.zeroCreate<int> nNotes
    let perd = Array.zeroCreate<int> nNotes
    let perdA = Array.zeroCreate<int> nNotes
    let hmax = Array.zeroCreate<int> nNotes
    let corrigerNote (note: int) =
        let f0 = f0Theorique note * g
        dure[note * 3 + 2] <- float32 f0
        let nbase = note * MaxTab * MaxPartiel
        let f0f = float32 f0
        for h in 0 .. MaxPartiel - 1 do
            let fv = f0f * float32 (h + 1)
            for i in 0 .. MaxTab - 1 do
                newFreq[nbase + i * MaxPartiel + h] <- fv
        let nb = int dure[note * 3 + 1]
        if nb > 0 then
            let avgF = frameStats freq nbase nb
            let avgA = frameStats amp nbase nb
            for h in 0 .. MaxPartiel - 1 do
                if avgF[h] > 0.0f && avgA[h] > seuilAmp then
                    let nH = Math.Round(float avgF[h] / f0, MidpointRounding.ToEven)
                    let harmNum = max 1 (int nH)
                    let cible = harmNum - 1
                    if cible >= MaxPartiel then
                        perd[note] <- perd[note] + 1
                        if maxFrames amp nbase h nb > 1e-3f then
                            perdA[note] <- perdA[note] + 1
                    else
                        if harmNum > hmax[note] then hmax[note] <- harmNum
                        let existante = slotMean newAmp nbase cible nb
                        if existante > 0.0f then
                            coll[note] <- coll[note] + 1
                        if avgA[h] > existante then
                            for i in 0 .. nb - 1 do
                                newAmp[nbase + i * MaxPartiel + cible] <-
                                    amp[nbase + i * MaxPartiel + h]
    (Array.init nNotes id) |> Array.Parallel.iter corrigerNote
    let st =
        { Collisions = Array.sum coll
          Perdues = Array.sum perd
          PerduesAudibles = Array.sum perdA
          HarmMax = Array.max hmax }
    newAmp, newFreq, st

// --- traiter (one siren) ----------------------------------------------------------------

let traiter (resources: string) (sortie: string option) (sirene: string) =
    let cheminAmp = Path.Combine(resources, $"dataAmp{sirene}")
    let taille = FileInfo(cheminAmp).Length
    if taille % (4L * int64 (MaxTab * MaxPartiel)) <> 0L then
        failwithf "%s: taille de fichier inattendue (%d)" sirene taille
    let nNotes = int (taille / (4L * int64 (MaxTab * MaxPartiel)))
    let sw = Stopwatch.StartNew()
    let amp = readF32 cheminAmp
    let freq = readF32 (Path.Combine(resources, $"dataFreq{sirene}"))
    let dure = readF32 (Path.Combine(resources, $"datadureTabs{sirene}"))
    let tRead = sw.Elapsed.TotalMilliseconds
    let grille = detecterGrille freq amp dure nNotes
    let tGrille = sw.Elapsed.TotalMilliseconds
    let newAmp, newFreq, st = corrigerSirene freq amp dure nNotes grille
    let tCorr = sw.Elapsed.TotalMilliseconds
    let tailleApres = int64 newAmp.Length * 4L + int64 newFreq.Length * 4L
    printfn
        "%s: %d notes, grille=f0x%s, harmonique max=%d, collisions=%d, harmoniques>%d perdues=%d (dont audibles >-60dB : %d), %d Mo -> %.1f Mo"
        sirene nNotes
        (if grille = 1.0 then "1.0" else "0.5")
        st.HarmMax st.Collisions MaxPartiel st.Perdues st.PerduesAudibles
        (taille * 2L / 1_000_000L) (float tailleApres / 1e6)
    match sortie with
    | Some sortie ->
        writeF32 (Path.Combine(sortie, $"dataAmp{sirene}")) newAmp
        writeF32 (Path.Combine(sortie, $"datadureTabs{sirene}")) dure
        writeF32 (Path.Combine(sortie, $"dataFreq{sirene}")) newFreq
    | None -> () // --dry-run
    let tWrite = sw.Elapsed.TotalMilliseconds
    printfn
        "PHASE %s: read %.0f | grille %.0f | corriger %.0f | write %.0f (ms)"
        sirene tRead (tGrille - tRead) (tCorr - tGrille) (tWrite - tCorr)

/// Fix every data set of `resources` into `sortie` (None: --dry-run, statistics only), then copy
/// the dataVectorInterval* files so the output directory is a complete drop-in.
let run (resources: string) (sortie: string option) =
    sortie |> Option.iter (Directory.CreateDirectory >> ignore)
    let sw = Stopwatch.StartNew()
    for s in sirenes do
        traiter resources sortie s
    match sortie with
    | Some sortie ->
        let swCopy = Stopwatch.StartNew()
        for f in Directory.GetFiles(resources, "dataVectorInterval*") do
            File.Copy(f, Path.Combine(sortie, Path.GetFileName f), true)
        printfn "PHASE copy: %.0f (ms)" swCopy.Elapsed.TotalMilliseconds
        printfn "Sortie drop-in complète dans : %s" sortie
    | None -> printfn "dry run: nothing written"
    printfn "TOTAL: %.2fs" sw.Elapsed.TotalSeconds
