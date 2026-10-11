//! The DSP's settings, one place for every switch of the Rust voice, keyed by id
//! (`frames.speed`, `trio.kernel`, ...), and the named variants built from them.
//!
//! These are app settings (global, not host-automatable plugin parameters). The defaults are the
//! plugin's own behaviour: a voice with `DspSettings::default()` applied is bit-exact with the C++
//! DSP (`tests/oracle.rs`).

use crate::sirene::{Edge, FrameControl, Gains, Kernel, LabFrames, Timing, Trio, Voice};
use std::fmt;

/// Every switch, with the plugin's behaviour as the default.
#[derive(Clone, Debug, PartialEq)]
pub struct DspSettings {
    /// `dsp.timing`: `plugin` (counted in samples, as the C++) or `time` (as at 44.1 kHz)
    pub timing: Timing,
    /// `frames.legacy`: the plugin's frame reset
    pub frames_legacy: bool,
    /// `frames.speed`, 0.25 to 4 (plugin frame reset off)
    pub frames_speed: f32,
    /// `frames.interpolate` (plugin frame reset off)
    pub frames_interpolate: bool,
    /// `synth.crossfade`: `None` keeps what the plugin parameter says
    pub crossfade: Option<bool>,
    /// `synth.bit16`: `None` keeps what the plugin parameter says
    pub bit16: Option<bool>,
    /// `synth.glide`: rotor inertia
    pub glide: bool,
    /// `synth.smoothing`, per sample at 44.1 kHz
    pub smoothing: f64,
    /// `trio.*`; `None`: the plugin's synthesis (`trio.kernel` = `off`)
    pub trio: Option<Trio>,
}

impl Default for DspSettings {
    fn default() -> Self {
        DspSettings {
            timing: Timing::Plugin,
            frames_legacy: true,
            frames_speed: 1.0,
            frames_interpolate: false,
            crossfade: None,
            bit16: None,
            glide: true,
            smoothing: 0.001,
            trio: None,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SettingError {
    pub key: String,
    pub value: String,
    pub reason: String,
}

impl fmt::Display for SettingError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} = {:?}: {}", self.key, self.value, self.reason)
    }
}

impl std::error::Error for SettingError {}

fn parse_bool(v: &str) -> Option<bool> {
    match v {
        "true" | "on" | "1" => Some(true),
        "false" | "off" | "0" => Some(false),
        _ => None,
    }
}

impl DspSettings {
    /// The switch ids `set` knows, in order.
    pub const KEYS: &'static [&'static str] = &[
        "dsp.timing",
        "frames.legacy",
        "frames.speed",
        "frames.interpolate",
        "synth.crossfade",
        "synth.bit16",
        "synth.glide",
        "synth.smoothing",
        "trio.kernel",
        "trio.gains",
        "trio.edge",
        "trio.scale_frames",
        "trio.lock",
        "trio.spacing",
        "trio.origin",
        "trio.envelope",
        "trio.envelope_at_harmonic",
    ];

    /// Sets one switch from its text value. A `trio.*` other than `trio.kernel` turns the trio on
    /// with its defaults if it was off.
    pub fn set(&mut self, key: &str, value: &str) -> Result<(), SettingError> {
        let err = |reason: &str| SettingError { key: key.into(), value: value.into(), reason: reason.into() };
        let b = || parse_bool(value).ok_or_else(|| err("expected true or false"));
        let num = |lo: f64, hi: f64| -> Result<f64, SettingError> {
            let x: f64 = value.parse().map_err(|_| err("expected a number"))?;
            if (lo..=hi).contains(&x) { Ok(x) } else { Err(err(&format!("out of range {lo} to {hi}"))) }
        };
        match key {
            "dsp.timing" => {
                self.timing = match value {
                    "plugin" => Timing::Plugin,
                    "time" => Timing::Time,
                    _ => return Err(err("expected plugin or time")),
                }
            }
            "frames.legacy" => self.frames_legacy = b()?,
            "frames.speed" => self.frames_speed = num(0.25, 4.0)? as f32,
            "frames.interpolate" => self.frames_interpolate = b()?,
            "synth.crossfade" => self.crossfade = Some(b()?),
            "synth.bit16" => self.bit16 = Some(b()?),
            "synth.glide" => self.glide = b()?,
            "synth.smoothing" => self.smoothing = num(0.0001, 0.01)?,
            "trio.kernel" => {
                let k = match value {
                    "off" => None,
                    "linear" => Some(Kernel::Linear2),
                    "b-spline" => Some(Kernel::BSpline3),
                    "nearest" => Some(Kernel::Nearest),
                    _ => return Err(err("expected off, linear, b-spline or nearest")),
                };
                self.trio = k.map(|kernel| Trio { kernel, ..self.trio.clone().unwrap_or_default() });
            }
            _ if key.starts_with("trio.") => {
                let t = self.trio.get_or_insert_with(Trio::default);
                match key {
                    "trio.gains" => {
                        t.gains = match value {
                            "linear" => Gains::Linear,
                            "power" => Gains::Power,
                            _ => return Err(err("expected linear or power")),
                        }
                    }
                    "trio.edge" => {
                        t.edge = match value {
                            "renormalize" => Edge::Renormalize,
                            "centre" => Edge::Centre,
                            "reflect" => Edge::Reflect,
                            _ => return Err(err("expected renormalize, centre or reflect")),
                        }
                    }
                    "trio.scale_frames" => t.scale_frames = b()?,
                    "trio.lock" => t.lock = b()?,
                    "trio.spacing" => t.spacing = num(1.0, 12.0)? as i32,
                    "trio.origin" => t.origin = num(0.0, 11.0)? as i32,
                    "trio.envelope" => t.envelope = num(0.0, 1.0)?,
                    "trio.envelope_at_harmonic" => t.harmonic_envelope = b()?,
                    _ => return Err(err("unknown setting")),
                }
            }
            _ => return Err(err("unknown setting")),
        }
        Ok(())
    }

    /// The settings of `pairs` (`key`, `value`) over the defaults.
    pub fn from_pairs<'a>(pairs: impl IntoIterator<Item = (&'a str, &'a str)>) -> Result<DspSettings, SettingError> {
        let mut s = DspSettings::default();
        for (k, v) in pairs {
            s.set(k, v)?;
        }
        Ok(s)
    }

    /// Puts the settings on a voice (before it plays: switching the trio resets its banks).
    pub fn apply(&self, v: &mut Voice) {
        v.set_timing(self.timing);
        v.glide = self.glide;
        let s = &mut v.siren;
        if let Some(on) = self.crossfade {
            s.set_crossfade(on);
        }
        if let Some(on) = self.bit16 {
            s.set_16ou8bit(!on);
        }
        if (self.smoothing - 0.001).abs() > 1e-12 {
            s.set_smoothing((self.smoothing, 1.0 - self.smoothing));
        }
        s.frames = if self.frames_legacy {
            FrameControl::Legacy
        } else {
            let mut l = LabFrames::full_loops(s.data());
            l.speed = self.frames_speed;
            l.interpolate = self.frames_interpolate;
            FrameControl::Lab(l)
        };
        s.set_trio(self.trio.clone());
    }
}

/// A named variant: an engine and the settings it runs with.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Variant {
    pub id: &'static str,
    /// `cpp`: the plugin's C++ DSP (nothing here runs); `rust`: this crate's voice
    pub engine: Engine,
    /// switch id = value pairs over [`DspSettings::default`]
    pub settings: &'static [(&'static str, &'static str)],
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Engine {
    Cpp,
    Rust,
}

impl Variant {
    pub fn dsp_settings(&self) -> Result<DspSettings, SettingError> {
        DspSettings::from_pairs(self.settings.iter().copied())
    }
}
