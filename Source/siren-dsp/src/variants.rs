// GENERATED from the shared metadata (siren-dsp-variants.csv, siren-datasets.csv and siren-dsp-switches.csv). Do not edit: change the metadata and regenerate.
//
// The DSP variants, the data sets and the switches of the voice.

use crate::settings::{Dataset, Engine, Processing, Switch, Variant};

/// Every variant; the first is the default.
pub const VARIANTS: &[Variant] = &[
    Variant {
        id: "cpp",
        label: "Plugin (C++)",
        engine: Engine::Cpp,
        settings: &[],
        description: "The plugin's own DSP, as it always was. The default.",
    },
    Variant {
        id: "rust-exact",
        label: "Rust port",
        engine: Engine::Rust,
        settings: &[],
        description: "The Rust port of the plugin's DSP, bit for bit the same sound.",
    },
    Variant {
        id: "rust-time",
        label: "Rust port (any rate)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time")],
        description: "The Rust port with its timers and frames in seconds: the same as the plugin at 44.1 kHz, right at other rates (the plugin's run 9 % fast at 48 kHz).",
    },
    Variant {
        id: "frames",
        label: "Frame loops",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false")],
        description: "Each note loops over all its frames instead of the plugin's first one or two.",
    },
    Variant {
        id: "trio",
        label: "Trio",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "b-spline"), ("trio.gains", "linear"), ("trio.scale_frames", "true"), ("trio.lock", "true")],
        description: "Three data notes around the pitch, quadratic B-spline weights, harmonics phase-locked: no ghost partials between notes.",
    },
    Variant {
        id: "trio-linear",
        label: "Trio (two notes)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "linear"), ("trio.gains", "linear"), ("trio.scale_frames", "true"), ("trio.lock", "true")],
        description: "The two data notes around the pitch, linear weights, phase-locked.",
    },
    Variant {
        id: "trio-nearest",
        label: "Nearest note",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "nearest"), ("trio.scale_frames", "true"), ("trio.lock", "true")],
        description: "The nearest data note alone, pitch-shifted.",
    },
    Variant {
        id: "trio-fast",
        label: "Trio (fast)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "b-spline"), ("trio.gains", "linear"), ("trio.scale_frames", "true"), ("trio.lock", "true"), ("trio.fast", "true")],
        description: "The trio with vectorized sines: the same sound within about 1e-9, several times cheaper.",
    },
    Variant {
        id: "trio-linear-fast",
        label: "Trio (two notes; fast)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "linear"), ("trio.gains", "linear"), ("trio.scale_frames", "true"), ("trio.lock", "true"), ("trio.fast", "true")],
        description: "The two-note trio with vectorized sines.",
    },
    Variant {
        id: "trio-nearest-fast",
        label: "Nearest note (fast)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "nearest"), ("trio.scale_frames", "true"), ("trio.lock", "true"), ("trio.fast", "true")],
        description: "The nearest note with vectorized sines.",
    },
    Variant {
        id: "trio-f32",
        label: "Trio (f32)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "b-spline"), ("trio.gains", "linear"), ("trio.scale_frames", "true"), ("trio.lock", "true"), ("trio.fast", "true"), ("trio.f32", "true")],
        description: "The trio with f32 harmonic phasors: within about 1e-5 of the trio, cheaper than the fast trio.",
    },
    Variant {
        id: "trio-linear-f32",
        label: "Trio (two notes; f32)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "linear"), ("trio.gains", "linear"), ("trio.scale_frames", "true"), ("trio.lock", "true"), ("trio.fast", "true"), ("trio.f32", "true")],
        description: "The two-note trio with f32 harmonic phasors.",
    },
    Variant {
        id: "trio-nearest-f32",
        label: "Nearest note (f32)",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false"), ("trio.kernel", "nearest"), ("trio.scale_frames", "true"), ("trio.lock", "true"), ("trio.fast", "true"), ("trio.f32", "true")],
        description: "The nearest note with f32 harmonic phasors.",
    },
];

/// Every data set; the first is the default.
pub const DATASETS: &[Dataset] = &[
    Dataset { id: "fixed", label: "Fixed harmonics", processing: Processing::FixData, description: "The recorded data with the crossfade fix: partials re-indexed by harmonic, frequencies set to exact harmonics. What the default build ships." },
    Dataset { id: "raw", label: "As measured", processing: Processing::None, description: "The recorded data files, untouched." },
];

/// The switches of a voice, with their default as text.
pub const SWITCHES: &[Switch] = &[
    Switch { id: "dsp.timing", default: "time" },
    Switch { id: "frames.legacy", default: "true" },
    Switch { id: "frames.speed", default: "1" },
    Switch { id: "frames.interpolate", default: "false" },
    Switch { id: "frames.skip_silent", default: "false" },
    Switch { id: "synth.crossfade", default: "false" },
    Switch { id: "synth.bit16", default: "false" },
    Switch { id: "synth.glide", default: "true" },
    Switch { id: "synth.smoothing", default: "0.001" },
    Switch { id: "trio.kernel", default: "b-spline" },
    Switch { id: "trio.gains", default: "linear" },
    Switch { id: "trio.edge", default: "renormalize" },
    Switch { id: "trio.scale_frames", default: "true" },
    Switch { id: "trio.lock", default: "true" },
    Switch { id: "trio.spacing", default: "1" },
    Switch { id: "trio.origin", default: "0" },
    Switch { id: "trio.envelope", default: "0" },
    Switch { id: "trio.envelope_at_harmonic", default: "false" },
    Switch { id: "trio.fast", default: "false" },
    Switch { id: "trio.f32", default: "false" },
];
