// The crossfade artifact fix for the siren data files: Resources/ -> a drop-in output directory.
//
//   dotnet fsi fix_data.fsx [--resources <dir>] [--output <dir>] [--dry-run]
//
// Defaults: --resources <repository>/Resources, --output <repository>/Resources-fixed.
// --dry-run prints the statistics and timings without writing anything. See FixData.fs.
#load "FixData.fs"

open System.IO

let root = Path.GetFullPath(Path.Combine(__SOURCE_DIRECTORY__, ".."))

let usage () =
    printfn "usage: dotnet fsi fix_data.fsx [--resources <dir>] [--output <dir>] [--dry-run]"
    printfn "  --resources <dir>  original data files (default: %s)" (Path.Combine(root, "Resources"))
    printfn "  --output <dir>     output directory (default: %s)" (Path.Combine(root, "Resources-fixed"))
    printfn "  --dry-run          print the statistics without writing"

let rec parse (resources, output, dry) args =
    match args with
    | "--resources" :: d :: rest -> parse (d, output, dry) rest
    | "--output" :: d :: rest -> parse (resources, d, dry) rest
    | "--dry-run" :: rest -> parse (resources, output, true) rest
    | ("-h" | "--help") :: _ -> usage (); exit 0
    | [] -> resources, output, dry
    | a :: _ -> eprintfn "unknown argument: %s" a; usage (); exit 2

let resources, output, dry =
    parse (Path.Combine(root, "Resources"), Path.Combine(root, "Resources-fixed"), false)
          (fsi.CommandLineArgs |> Array.skip 1 |> List.ofArray)

FixData.run resources (if dry then None else Some output)
