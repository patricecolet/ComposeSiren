//! The named DSP variants.
use crate::settings::{Engine, Variant};

/// Every variant; the first is the default (the plugin's C++ DSP).
pub const VARIANTS: &[Variant] = &[
    Variant { id: "cpp", engine: Engine::Cpp, settings: &[] },
    Variant { id: "rust-exact", engine: Engine::Rust, settings: &[] },
    Variant { id: "rust-time", engine: Engine::Rust, settings: &[("dsp.timing", "time")] },
    Variant {
        id: "frames",
        engine: Engine::Rust,
        settings: &[("dsp.timing", "time"), ("frames.legacy", "false")],
    },
    Variant {
        id: "trio",
        engine: Engine::Rust,
        settings: &[
            ("dsp.timing", "time"),
            ("frames.legacy", "false"),
            ("trio.kernel", "b-spline"),
            ("trio.gains", "linear"),
            ("trio.scale_frames", "true"),
            ("trio.lock", "true"),
        ],
    },
    Variant {
        id: "trio-linear",
        engine: Engine::Rust,
        settings: &[
            ("dsp.timing", "time"),
            ("frames.legacy", "false"),
            ("trio.kernel", "linear"),
            ("trio.gains", "linear"),
            ("trio.scale_frames", "true"),
            ("trio.lock", "true"),
        ],
    },
    Variant {
        id: "trio-nearest",
        engine: Engine::Rust,
        settings: &[
            ("dsp.timing", "time"),
            ("frames.legacy", "false"),
            ("trio.kernel", "nearest"),
            ("trio.scale_frames", "true"),
            ("trio.lock", "true"),
        ],
    },
];

/// The variant called `id`.
pub fn variant(id: &str) -> Option<&'static Variant> {
    VARIANTS.iter().find(|v| v.id == id)
}
