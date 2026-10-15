//! A port of the plugin's `Sirene` (calculwave, setMidicent, setnote), driven
//! by its `MidiIn` (`midi_in.rs`) as `SirenVoiceUnit` drives it.
//!
//! It is meant to be sample-exact with the C++ (see `tests/oracle.rs`), so it
//! keeps the C++'s float/double promotions, written out where they happen:
//! `float * float` stays `f32`, anything divided by `100.` is `f64`, and an
//! `f32 +=` of a `f64` expression adds in `f64` and rounds back to `f32`.
//!
//! The lab adds one thing, [`FrameControl::Lab`]: which frames each note
//! plays. [`FrameControl::Legacy`] is the plugin, where `setMidicent()`,
//! called every millisecond, puts the frame counters back to 0 each time, so
//! a note never gets past its first frame or two.

use std::sync::Arc;

use crate::data::{DataSet, MAX_PARTIEL, NOMBRE_DE_NOTE, VectorInterval};
use crate::midi_in::{MidiIn, Sink};
use crate::siren::SirenDef;

/// The C++ indexes `[noteSup]` = 80 at the top of the soprano and piccolo
/// range, one past its arrays (undefined behaviour there). One more slot here.
const NOTES: usize = NOMBRE_DE_NOTE + 1;

/// The rate the data's frame durations are counted at, and the plugin's.
pub const DATA_RATE: f64 = 44100.0;

/// How the DSP counts time at a sample rate other than [`DATA_RATE`]. The
/// plugin counts in samples: its frames last `fdur` samples, `timerAudio` (the
/// vibrato, tremolo, portamento, attack and release steps) runs every 512
/// samples, `setnote` every `sr / 1000` (truncated: 44 at 44.1 kHz), the
/// clapet moves 0.0002 per sample and the amplitudes smooth by 0.001 per
/// sample, so at 48 kHz most of these run 9 % fast (the vibrato's own rate is
/// corrected, its depth envelope is not). The two are the same at 44.1 kHz,
/// sample for sample.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Timing {
    /// every timer, ramp and frame lasts what it lasts at 44.1 kHz
    #[default]
    Time,
    /// counted in samples, as the plugin does
    Plugin,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct FrameSel {
    /// first frame of the loop
    pub start: usize,
    /// one past the last frame of the loop
    pub end: usize,
    /// play this one frame only
    pub freeze: Option<usize>,
}

#[derive(Clone, Debug)]
pub struct LabFrames {
    /// per note index
    pub sel: Vec<FrameSel>,
    /// frames per `fdur` samples: 2.0 plays the loop twice as fast
    pub speed: f32,
    /// read between frames linearly instead of stepping
    pub interpolate: bool,
}

#[derive(Clone, Debug)]
pub enum FrameControl {
    /// the plugin: frame counters reset by every `setMidicent()`
    Legacy,
    /// the lab: counters reset only when the note changes, loops per note
    Lab(LabFrames),
}

impl LabFrames {
    /// Every note loops on `[0, fmax)`, like the plugin would if its
    /// counters were not reset.
    pub fn full_loops(data: &DataSet) -> LabFrames {
        LabFrames {
            sel: (0..NOTES)
                .map(|n| FrameSel {
                    start: 0,
                    end: data.info.get(n).map_or(0, |i| i.fmax),
                    freeze: None,
                })
                .collect(),
            speed: 1.0,
            interpolate: false,
        }
    }
}

/// The three-note crossfade (`Sirene::trio`): instead of mixing the
/// frequencies of partial slot i of two notes (the plugin, which makes ghost
/// partials where slot i is not the same harmonic in both), three banks of
/// oscillators play the nearest data note and its two neighbours, each with
/// all its partials pitch-shifted to the target pitch, and only their outputs
/// are mixed, by `kernel` weights over the semitone distance.
#[derive(Clone, Debug, PartialEq)]
pub struct Trio {
    pub kernel: Kernel,
    pub gains: Gains,
    pub edge: Edge,
    /// lab frames only: a bank's frames last `fdur / ratio`, so the frames of a
    /// note shifted up go by faster, as they would at that pitch (frames are
    /// tied to the period)
    pub scale_frames: bool,
    /// per data note, how many cents its partials sit above its nominal pitch
    /// (measured from the data); the pitch shift then lands them on the target.
    /// Empty: the nominal pitch, as the plugin does
    pub tuning: Vec<f32>,
    /// Harmonic phase lock: every bank's partial is given its harmonic number h
    /// of the target pitch, and all banks' harmonic h run on one phase, h·Φ.
    /// The neighbours then add in phase instead of beating against each other
    /// (their partials sit within cents of each other, with unrelated phases),
    /// and `Gains::Linear` keeps the amplitude. The partials' frequencies are
    /// then exact harmonics: the data's small inharmonicity is dropped.
    pub lock: bool,
    /// the data set's pitch grid against the nominal pitches, in octaves (the
    /// piccolo's data sounds one octave up): with `lock`, the target fundamental
    /// is the target pitch times 2^octave
    pub octave: i32,
    /// A sparse data set: only the anchors play, the data notes `origin + j ·
    /// spacing`; the kernel runs over the anchors (its distance in units of
    /// `spacing` semitones), so a bank is shifted by up to 1.5 · `spacing`
    /// semitones. 1: every data note
    pub spacing: i32,
    pub origin: i32,
    /// Envelope correction, 0 to 1: each partial's amplitude times
    /// (E(f · ratio) / E(f))^envelope, E the spectral envelope of its note
    /// ([`Envelope`]). 0: the levels follow the harmonic number (a source
    /// scaling with the period); 1: they stay where they are in frequency (a
    /// fixed filter, which sounds closer). The note's f0 for E
    /// is its nominal pitch with `tuning` and `octave`
    pub envelope: f64,
    /// With `lock`: read the envelope correction at the harmonic each partial
    /// sounds on (h·F at the target, h·F/ratio in its note) instead of at the
    /// slot's own frequency, so the frames' frequency jitter and slot swaps do
    /// not move the correction
    pub harmonic_envelope: bool,
    /// The fast path: the phases kept within a few turns and their sines from
    /// [`crate::fastsin`] (vectorized) instead of libm's. Not bit-exact with the
    /// trio without it: within the bound `tests/fast.rs` checks
    pub fast: bool,
    /// With `fast` and `lock`: the sines of the harmonics h·Φ from f32 phasors (one polynomial
    /// cos/sin of Φ per computed sample, as the kernels' `anchor`, then e^{ihΦ} by complex
    /// products, eight harmonics per step) instead of one sine per partial. Looser than `fast`:
    /// the bound is `tests/fast.rs`'s
    pub f32_phasors: bool,
}

impl Default for Trio {
    fn default() -> Trio {
        Trio {
            kernel: Kernel::BSpline3,
            gains: Gains::Power,
            edge: Edge::Renormalize,
            scale_frames: false,
            tuning: Vec::new(),
            lock: false,
            octave: 0,
            spacing: 1,
            origin: 0,
            envelope: 0.0,
            harmonic_envelope: false,
            fast: false,
            f32_phasors: false,
        }
    }
}

/// The weights of the notes around the target pitch `p` (in semitones).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kernel {
    /// the two notes around `p`, linearly: the plugin's crossfade weights, or
    /// the two banks of the original piccolo (`calculwavePiccolo`)
    Linear2,
    /// three notes: a quadratic B-spline of the distance `d = p - n`,
    /// `0.75 - d²` within half a semitone, `(1.5 - |d|)² / 2` up to 1.5: the
    /// nearest note and its two neighbours always sound, the weights sum to 1
    /// and change smoothly (continuous slope) when the nearest note changes
    BSpline3,
    /// the nearest note alone, pitch-shifted (a sampler with one sample per
    /// key zone): the baseline for sparse data
    Nearest,
}

/// From weights (summing to 1) to gains.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Gains {
    /// the weights: the amplitudes sum to 1 (right for signals in phase)
    Linear,
    /// their square roots: the powers sum to 1 (right for unrelated phases)
    Power,
}

/// What a missing note's weight becomes, at the ends of the range where the
/// neighbour has no data: the total stays 1 in each case.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Edge {
    /// the notes that exist share it, in proportion to their weights
    Renormalize,
    /// it goes to the nearest note
    Centre,
    /// it goes to the other neighbour (a mirror around the nearest note)
    Reflect,
}

/// One oscillator bank of the trio: a data note, its frame position and its
/// oscillators. Four banks, kept per note (slot `note mod 4`): the three notes
/// sounding, and the one that just left, fading out at gain 0 through the
/// amplitude smoothing, so no bank is cut while it is still heard (with
/// equal-power gains the leaving note still has 0.7 % one cent before the
/// switch).
#[derive(Clone)]
struct Bank {
    note: i32,
    frame: i32,
    count_k: i32,
    /// [`Clock::carry`]
    carry: f64,
    gain: f64,
    /// target pitch over the note's pitch
    ratio: f64,
    /// radians per sample per Hz of the note's data
    ps: f64,
    phase: [f64; MAX_PARTIEL],
    ampz: [f64; MAX_PARTIEL],
    /// `Trio::envelope`: the amplitude factors of the partials of `corr_frame`
    /// at the current ratio (`i32::MIN`: to compute)
    corr: [f64; MAX_PARTIEL],
    corr_frame: i32,
    /// `Trio::f32_phasors`: every partial's harmonic number (kept in `phase`, 0 past
    /// [`TRIO_HARMONICS`]) is i + 1 (the fixed data: slot i is harmonic i + 1): the sines are a slice
    in_order: bool,
}

impl Bank {
    fn new() -> Bank {
        Bank {
            note: -1,
            frame: 0,
            count_k: 0,
            carry: 0.0,
            gain: 0.0,
            ratio: 1.0,
            ps: 0.0,
            phase: [0.0; MAX_PARTIEL],
            ampz: [0.0; MAX_PARTIEL],
            corr: [1.0; MAX_PARTIEL],
            corr_frame: i32::MIN,
            in_order: false,
        }
    }
}

/// A note's spectral envelope, for `Trio::envelope`: the level of each harmonic
/// over the note's loop (the energy of the slots nearest to it, from their
/// mean amplitude and amplitude-weighted mean frequency), joined linearly in dB over log-frequency
/// through the harmonics that have energy (a missing harmonic is a gap
/// bridged, not a notch), flat past the first and the last.
#[derive(Clone, Debug, PartialEq)]
pub struct Envelope {
    pub f0: f64,
    /// (harmonic number, level in dB), increasing harmonic numbers
    pub points: Vec<(f64, f64)>,
}

impl Envelope {
    pub fn of(data: &DataSet, n: usize, f0: f64) -> Option<Envelope> {
        let info = data.info.get(n)?;
        let l = if info.fmax > 0 { info.fmax.min(info.data_frames.max(1)) } else { info.data_frames };
        if l == 0 || f0 <= 0.0 {
            return None;
        }
        let mut energy = vec![0.0f64; MAX_PARTIEL + 1];
        for i in 0..MAX_PARTIEL {
            let (mut sa, mut sf) = (0.0f64, 0.0f64);
            for fr in 0..l {
                let a = data.amp(n, fr)[i] as f64;
                sa += a;
                sf += a * data.freq(n, fr)[i] as f64;
            }
            if sa > 0.0 {
                let k = (sf / sa / f0).round();
                if k >= 1.0 && k <= MAX_PARTIEL as f64 {
                    energy[k as usize] += (sa / l as f64).powi(2);
                }
            }
        }
        let points: Vec<(f64, f64)> =
            (1..=MAX_PARTIEL).filter(|&k| energy[k] > 0.0).map(|k| (k as f64, 10.0 * energy[k].log10())).collect();
        if points.is_empty() { None } else { Some(Envelope { f0, points }) }
    }

    /// the level in dB at `hz`
    pub fn at(&self, hz: f64) -> f64 {
        let r = hz / self.f0;
        let p = &self.points;
        if r <= p[0].0 {
            return p[0].1;
        }
        if r >= p[p.len() - 1].0 {
            return p[p.len() - 1].1;
        }
        let k = p.partition_point(|&(h, _)| h <= r);
        let ((h0, d0), (h1, d1)) = (p[k - 1], p[k]);
        let t = (r.ln() - h0.ln()) / (h1.ln() - h0.ln());
        d0 + t * (d1 - d0)
    }
}

pub struct Sirene {
    data: Arc<DataSet>,
    vector_interval: [f32; 392],
    note_midi_cent_max: i32,
    note_min: i32,
    pourcent_clapet_off: i32,
    coeff_picolo: i32,

    sample_rate: f64,
    deux_pie_sample_rate: f64,
    timing: Timing,
    /// samples per sample at 44.1 kHz: the sample rate over [`DATA_RATE`] with
    /// [`Timing::Time`], 1 with [`Timing::Plugin`]
    time_scale: f64,

    pub frames: FrameControl,
    /// `frames.skip_silent`: a practically silent frame plays the next one that is not
    /// ([`DataSet::play_frame`]); off, the plugin's frames bit for bit
    pub skip_silent: bool,
    /// amplitude smoothing at 44.1 kHz, `ampz = a * amp + b * ampz` (0.001, 0.999)
    smoothing: (f64, f64),
    /// the same per sample at this rate (`smoothing` when `time_scale` is 1)
    smoothing_rate: (f64, f64),

    count8bit: bool,
    vitesse_clape: f64,
    count_k_inf: i32,
    count_k_sup: i32,
    midi_cent_voulue: i32,
    ancien_note_voulue: i32,
    qualite: i32,
    phase_inf: [f64; MAX_PARTIEL],
    count_p: [i32; NOTES],
    /// per note, [`Clock::carry`]
    frame_carry: [f64; NOTES],
    pitch_schift: [f32; NOTES],
    ancienne_wave_inf: f32,
    eloignementfreq: i32,
    note_inf: i32,
    note_sup: i32,
    ampvoulu: f32,
    ampvouluz: f32,
    is_changement_de_note: bool,
    ampz: [f32; MAX_PARTIEL],
    amp: [f32; MAX_PARTIEL],
    is16bit: bool,
    note_voulue_avant_slide: i32,
    note_encour: f32,
    inter_depart: i32,
    is_crossfade: bool,

    /// the three-note crossfade, in place of the plugin's (see [`Trio`])
    trio: Option<Trio>,
    banks: [Bank; 4],
    trio_notes: [(i32, f64); 3],
    /// `note_hz` of MIDI notes 0..128 for the trio, computed in `set_trio` (the same
    /// values: the glide changes the pitch every sample, and these are seven `powf`)
    trio_hz: [f64; 128],
    /// `Trio::f32_phasors`: sin(h·Φ) for h below [`TRIO_HARMONICS`] at the current sample
    trio_sines: [f32; TRIO_HARMONICS],
    trio_mc: i32,
    trio_wave: f64,
    /// with `Trio::lock`: the phase of the target fundamental, and its step
    trio_phi: f64,
    trio_dphi: f64,
    /// set by `set_handoff`: the banks the next pitch change makes start on the carried
    /// frames, at their smoothed level (a held note taken over plays at once at its level)
    trio_prime: bool,
    trio_f0: f64,
    /// with `Trio::envelope`: each data note's envelope
    envelopes: Vec<Option<Envelope>>,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Slide {
    Montant,
    Descandant,
    TonUpBefore,
    DemiUpBefore,
    QuartUpBefore,
    Boucle,
    QuartDownAfter,
    QuartDownBefore,
    QuartUpAfter,
}

impl Sirene {
    pub fn new(def: &SirenDef, data: Arc<DataSet>, vi: &VectorInterval) -> Sirene {
        let c = def.constants;
        let mut s = Sirene {
            data,
            vector_interval: vi.0,
            note_midi_cent_max: c.note_midi_cent_max,
            note_min: c.note_min,
            pourcent_clapet_off: c.pourcent_clapet_off,
            coeff_picolo: c.coeff_picolo,
            sample_rate: 0.0,
            deux_pie_sample_rate: 0.0,
            timing: Timing::Time,
            time_scale: 1.0,
            frames: FrameControl::Legacy,
            skip_silent: false,
            smoothing: (0.001, 0.999),
            smoothing_rate: (0.001, 0.999),
            count8bit: true,
            vitesse_clape: 0.0002,
            count_k_inf: 0,
            count_k_sup: 0,
            midi_cent_voulue: 0,
            ancien_note_voulue: 0,
            qualite: 30,
            phase_inf: [0.0; MAX_PARTIEL],
            count_p: [0; NOTES],
            frame_carry: [0.0; NOTES],
            pitch_schift: [0.0; NOTES],
            ancienne_wave_inf: 0.0,
            eloignementfreq: 0,
            note_inf: 0,
            note_sup: 0,
            ampvoulu: 1.0,
            ampvouluz: 1.0,
            is_changement_de_note: false,
            ampz: [0.0; MAX_PARTIEL],
            amp: [0.0; MAX_PARTIEL],
            is16bit: false,
            note_voulue_avant_slide: 0,
            note_encour: 0.0,
            inter_depart: 0,
            is_crossfade: false,
            trio: None,
            banks: [Bank::new(), Bank::new(), Bank::new(), Bank::new()],
            trio_notes: [(-1, 0.0); 3],
            trio_hz: [0.0; 128],
            trio_sines: [0.0; TRIO_HARMONICS],
            trio_mc: i32::MIN,
            trio_wave: 0.0,
            trio_phi: 0.0,
            trio_dphi: 0.0,
            trio_prime: false,
            trio_f0: 0.0,
            envelopes: Vec::new(),
        };
        s.set_sample_rate(44100.0);
        s
    }

    pub fn data(&self) -> &DataSet {
        &self.data
    }

    pub fn set_sample_rate(&mut self, sr: f64) {
        self.sample_rate = sr;
        self.deux_pie_sample_rate = (2.0 * std::f64::consts::PI) / sr;
        self.time_scale = match self.timing {
            Timing::Time => sr / DATA_RATE,
            Timing::Plugin => 1.0,
        };
        // 0.0002 per sample at 44.1 kHz
        self.vitesse_clape = 0.0002 / self.time_scale;
        self.set_smoothing(self.smoothing);
        if self.midi_cent_voulue > 0 {
            self.set_midicent(self.midi_cent_voulue);
        }
    }

    pub fn set_timing(&mut self, timing: Timing) {
        self.timing = timing;
        self.set_sample_rate(self.sample_rate);
    }

    pub fn timing(&self) -> Timing {
        self.timing
    }

    /// samples per sample at 44.1 kHz (1 with [`Timing::Plugin`])
    pub fn time_scale(&self) -> f64 {
        self.time_scale
    }

    /// The amplitude smoothing `(a, b)` per sample at 44.1 kHz; with
    /// [`Timing::Time`], the same time constant at any rate: b^(44.1 kHz / rate)
    pub fn set_smoothing(&mut self, (a, b): (f64, f64)) {
        self.smoothing = (a, b);
        self.smoothing_rate = if self.time_scale == 1.0 {
            (a, b)
        } else {
            let b1 = b.powf(1.0 / self.time_scale);
            // a keeps its ratio to 1 - b (unity gain when a + b = 1)
            (if b < 1.0 { a * (1.0 - b1) / (1.0 - b) } else { a }, b1)
        };
    }

    pub fn smoothing(&self) -> (f64, f64) {
        self.smoothing
    }

    pub fn change_qualite(&mut self, q: i32) {
        self.qualite = q.clamp(0, MAX_PARTIEL as i32);
    }

    /// `set16ou8Bit(bool is)`: `is` = 8-bit
    pub fn set_16ou8bit(&mut self, is_8bit: bool) {
        self.is16bit = !is_8bit;
    }

    pub fn set_crossfade(&mut self, on: bool) {
        self.is_crossfade = on;
    }

    /// The three-note crossfade, or `None` for the plugin's synthesis.
    pub fn set_trio(&mut self, trio: Option<Trio>) {
        self.envelopes = match &trio {
            Some(t) if t.envelope != 0.0 => (0..NOMBRE_DE_NOTE as i32)
                .map(|n| if self.has_data(n) { Envelope::of(&self.data, n as usize, note_hz(n, t) * 2f64.powi(t.octave)) } else { None })
                .collect(),
            _ => Vec::new(),
        };
        if let Some(t) = &trio {
            for (n, hz) in self.trio_hz.iter_mut().enumerate() {
                *hz = note_hz(n as i32, t);
            }
        }
        self.trio = trio;
        for b in self.banks.iter_mut() {
            b.note = -1;
            b.gain = 0.0;
        }
        self.trio_notes = [(-1, 0.0); 3];
        self.trio_mc = i32::MIN;
    }

    /// `Trio::envelope`'s envelope of a data note
    pub fn envelope(&self, note: usize) -> Option<&Envelope> {
        self.envelopes.get(note).and_then(|e| e.as_ref())
    }

    pub fn trio(&self) -> Option<&Trio> {
        self.trio.as_ref()
    }

    /// The trio's three notes and their gains, lowest note first.
    pub fn trio_state(&self) -> [(i32, f64); 3] {
        self.trio_notes
    }

    pub fn set_velocite(&mut self, velo: i32) {
        let pc = self.pourcent_clapet_off;
        self.ampvoulu = ((velo as f64 / 500.0) / (100.0 / (100 - pc) as f64) + (pc as f64 / 100.0)) as f32;
    }

    /// Lab: fade to silence (the plugin's note off leaves the clapet-off
    /// level, 7 % or 15 %).
    pub fn silence(&mut self) {
        self.ampvoulu = 0.0;
    }

    pub fn current_pitch(&self) -> f32 {
        self.note_encour
    }

    pub fn note_inf(&self) -> usize {
        self.note_inf.max(0) as usize
    }

    /// the frame each note is on, for the UI
    pub fn frame_of(&self, note: usize) -> usize {
        if self.trio.is_some()
            && let Some(b) = self.banks.iter().find(|b| b.note == note as i32) {
                return b.frame.max(0) as usize;
            }
        self.count_p.get(note).map_or(0, |&p| p.max(0) as usize)
    }

    pub fn set_midicent(&mut self, note: i32) {
        self.midi_cent_voulue = note;
        if self.midi_cent_voulue >= self.note_midi_cent_max {
            self.midi_cent_voulue = self.note_midi_cent_max;
        } else if self.midi_cent_voulue % 100 == 99 {
            self.midi_cent_voulue += 1;
        }

        let new_note_inf = self.midi_cent_voulue / 100;
        let reset = match &self.frames {
            FrameControl::Legacy => true,
            FrameControl::Lab(_) => new_note_inf != self.note_inf,
        };
        if reset {
            self.note_inf = new_note_inf;
            self.note_sup = self.note_inf + 1;
            let (inf, sup) = (self.note_inf, self.note_sup);
            self.count_p[idx(inf)] = self.first_frame(inf);
            self.count_p[idx(sup)] = self.first_frame(sup);
            self.count_k_inf = 0;
            self.count_k_sup = 0;
            self.frame_carry[idx(inf)] = 0.0;
            self.frame_carry[idx(sup)] = 0.0;
            if matches!(self.frames, FrameControl::Legacy) {
                // the millisecond reset holds the trio's banks on frame 0 too
                for b in self.banks.iter_mut() {
                    b.frame = 0;
                    b.count_k = 0;
                    b.carry = 0.0;
                }
            }
        }

        let wanted = 440.0 * 2f64.powf(((self.midi_cent_voulue as f64 / 100.0) - 69.0) / 12.0);
        let at = |n: i32| 440.0 * 2f64.powf((n as f64 - 69.0) / 12.0);
        self.pitch_schift[idx(self.note_inf)] = ((wanted / at(self.note_inf)) * self.deux_pie_sample_rate) as f32;
        self.pitch_schift[idx(self.note_sup)] = ((wanted / at(self.note_sup)) * self.deux_pie_sample_rate) as f32;
    }

    fn first_frame(&self, note: i32) -> i32 {
        match &self.frames {
            FrameControl::Legacy => 0,
            FrameControl::Lab(l) => match l.sel.get(idx(note)) {
                Some(s) => s.freeze.unwrap_or(s.start) as i32,
                None => 0,
            },
        }
    }

    pub fn set_note_from_ext(&mut self, note: i32) {
        self.note_voulue_avant_slide = note;
        if self.note_voulue_avant_slide > self.note_midi_cent_max {
            self.note_voulue_avant_slide = self.note_midi_cent_max;
        }
        self.inter_depart = (self.note_voulue_avant_slide as f32 - self.note_encour) as i32;
        if self.note_encour > self.note_voulue_avant_slide as f32 {
            self.note_encour -= 1.0;
        } else if self.note_encour < self.note_voulue_avant_slide as f32 {
            self.note_encour += 1.0;
        }
    }

    /// Lab: go to a pitch at once, without the rotor's inertia.
    pub fn jump_to(&mut self, midicent: i32) {
        self.note_voulue_avant_slide = midicent.min(self.note_midi_cent_max);
        self.note_encour = self.note_voulue_avant_slide as f32;
        self.inter_depart = 0;
        self.set_midicent(self.note_encour as i32);
    }

    /// The midicent that `setMidicent` last settled on.
    pub fn midicent(&self) -> i32 {
        self.midi_cent_voulue
    }

    fn oujesuis(&self) -> Slide {
        let inter = (self.note_voulue_avant_slide as f32 - self.note_encour) as i32;
        let d = inter - self.inter_depart;
        if inter == 0 {
            Slide::Boucle
        } else if d > 0 && d < 50 {
            Slide::QuartDownAfter
        } else if d < 0 && d > -50 {
            Slide::QuartUpAfter
        } else if (-50..0).contains(&inter) {
            Slide::QuartDownBefore
        } else if inter > 250 && d <= -50 {
            Slide::Montant
        } else if inter < -50 && d >= 50 {
            Slide::Descandant
        } else if (150..250).contains(&inter) {
            Slide::TonUpBefore
        } else if (50..150).contains(&inter) {
            Slide::DemiUpBefore
        } else if inter > 0 && inter < 50 {
            Slide::QuartUpBefore
        } else {
            Slide::Boucle
        }
    }

    /// `setnote()`, the "méthode Pat": the rotor's slide towards the wanted
    /// pitch, at the rates of `vectorInterval`.
    pub fn setnote(&mut self) {
        let ou = self.oujesuis();
        let factor = self.coeff_picolo as f32;
        let mut note = ((self.note_encour - 50.0) / 100.0) as i32;
        if note < self.note_min {
            note = self.note_min;
        }
        let base = (note - self.note_min) as usize;
        let vi = &self.vector_interval;
        let target = self.note_voulue_avant_slide as f32;
        let step = |i: usize| 100.0f32 / (vi[i] * factor);
        match ou {
            Slide::Montant => {
                self.note_encour += step(base + 294);
                if self.note_encour > target {
                    self.note_encour = target;
                }
            }
            Slide::Descandant => {
                self.note_encour -= step(391 - base);
                if self.note_encour < target {
                    self.note_encour = target;
                }
            }
            Slide::TonUpBefore => self.note_encour += step((base + 2) * 6 + 1),
            Slide::DemiUpBefore => self.note_encour += step((base + 1) * 6 + 2),
            Slide::QuartUpBefore => {
                self.note_encour += step(base * 6 + 3);
                if self.note_encour > target {
                    self.note_encour = target;
                }
            }
            Slide::Boucle => {}
            Slide::QuartDownAfter => {
                self.note_encour -= step(base * 6 + 4);
                if self.note_encour < target {
                    self.note_encour = target;
                }
            }
            Slide::QuartDownBefore => {
                self.note_encour -= step(base * 6);
                if self.note_encour < target {
                    self.note_encour = target;
                }
            }
            Slide::QuartUpAfter => {
                self.note_encour += step(base * 6 + 5);
                if self.note_encour > target {
                    self.note_encour = target;
                }
            }
        }
        self.set_midicent(self.note_encour as i32);
    }

    #[inline]
    fn advance(&mut self, note: i32, count_k: i32) -> i32 {
        let n = idx(note);
        let clock = Clock { rate: 1.0, time_scale: self.time_scale };
        advance_frame(&self.data, &self.frames, n, &mut self.count_p[n], count_k, clock, &mut self.frame_carry[n])
    }

    /// `(freq, amp)` of partial `i` of `note`, between frames when the lab
    /// interpolates.
    #[inline]
    fn partial(&self, note: i32, count_k: i32, i: usize) -> (f32, f32) {
        let n = idx(note);
        let clock = Clock { rate: 1.0, time_scale: self.time_scale };
        partial_at(&self.data, &self.frames, self.skips(), n, self.count_p[n], count_k, i, clock, self.frame_carry[n])
    }

    /// `calculwave()`: one output sample.
    pub fn calculwave(&mut self) -> f32 {
        if self.trio.is_some() {
            return self.calculwave_trio();
        }
        self.is_changement_de_note = false;
        if self.note_inf != self.ancien_note_voulue {
            self.is_changement_de_note = true;
            self.ancien_note_voulue = self.note_inf;
        }

        self.count_k_inf = self.advance(self.note_inf, self.count_k_inf);
        self.count_k_sup = self.advance(self.note_sup, self.count_k_sup);

        if self.ampvouluz < self.ampvoulu {
            self.ampvouluz = (self.ampvouluz as f64 + self.vitesse_clape) as f32;
        }
        if self.ampvouluz > self.ampvoulu {
            self.ampvouluz = (self.ampvouluz as f64 - self.vitesse_clape) as f32;
        }

        let mut wave_inf = 0.0f32;
        self.eloignementfreq = self.note_sup * 100 - self.midi_cent_voulue;
        self.count8bit = !self.count8bit;

        let (inf, sup) = (self.note_inf, self.note_sup);
        let ps_inf = self.pitch_schift[idx(inf)];
        let ps_sup = self.pitch_schift[idx(sup)];
        let el = self.eloignementfreq as f32;
        let el_sup = (100 - self.eloignementfreq) as f32;
        let (sa, sb) = self.smoothing_rate;
        let compute = self.is16bit || self.is_changement_de_note || self.count8bit;

        if !matches!(&self.frames, FrameControl::Lab(l) if l.interpolate) {
            // Frames read whole (the plugin's): the arithmetic of the loop below, in the same
            // order, with each note's frame looked up once per sample instead of per partial.
            let q = self.qualite as usize;
            let (ni, ns) = (idx(inf), idx(sup));
            let skip = self.skips();
            let pi = self.data.play_frame(ni, self.count_p[ni].max(0) as usize, skip);
            let psu = self.data.play_frame(ns, self.count_p[ns].max(0) as usize, skip);
            let (fi, ai) = (&self.data.freq(ni, pi)[..q], &self.data.amp(ni, pi)[..q]);
            let (fs, as_) = (&self.data.freq(ns, psu)[..q], &self.data.amp(ns, psu)[..q]);
            let phase = &mut self.phase_inf[..q];
            if compute {
                let (amp, ampz) = (&mut self.amp[..q], &mut self.ampz[..q]);
                let crossfade = self.is_crossfade;
                for i in 0..q {
                    if crossfade {
                        phase[i] += ((fi[i] * ps_inf) * el) as f64 / 100.0 + ((fs[i] * ps_sup) * el_sup) as f64 / 100.0;
                        amp[i] = ((ai[i] * el) as f64 / 100.0 + (as_[i] * el_sup) as f64 / 100.0) as f32;
                    } else {
                        amp[i] = ai[i];
                        phase[i] += (fi[i] * ps_inf) as f64;
                    }
                    ampz[i] = (sa * amp[i] as f64 + sb * ampz[i] as f64) as f32;
                    wave_inf = (wave_inf as f64 + phase[i].sin() * ampz[i] as f64) as f32;
                    if phase[i] == 180.0 {
                        phase[i] = 0.0;
                    }
                }
                if q > 0 {
                    self.ancienne_wave_inf = wave_inf;
                }
            } else {
                for i in 0..q {
                    phase[i] += (fi[i] * ps_inf) as f64;
                }
            }
        } else {
        for i in 0..self.qualite as usize {
            let (f_inf, a_inf) = self.partial(inf, self.count_k_inf, i);
            if compute {
                if self.is_crossfade {
                    let (f_sup, a_sup) = self.partial(sup, self.count_k_sup, i);
                    self.phase_inf[i] += ((f_inf * ps_inf) * el) as f64 / 100.0 + ((f_sup * ps_sup) * el_sup) as f64 / 100.0;
                    self.amp[i] = ((a_inf * el) as f64 / 100.0 + (a_sup * el_sup) as f64 / 100.0) as f32;
                } else {
                    self.amp[i] = a_inf;
                    self.phase_inf[i] += (f_inf * ps_inf) as f64;
                }
                self.ampz[i] = (sa * self.amp[i] as f64 + sb * self.ampz[i] as f64) as f32;
                wave_inf = (wave_inf as f64 + self.phase_inf[i].sin() * self.ampz[i] as f64) as f32;
                self.ancienne_wave_inf = wave_inf;
                if self.phase_inf[i] == 180.0 {
                    self.phase_inf[i] = 0.0;
                }
            } else {
                // the C++ advances with noteInf only here, crossfade or not
                self.phase_inf[i] += (f_inf * ps_inf) as f64;
            }
        }
        }

        let mut out = if self.is16bit { wave_inf * self.ampvouluz } else { self.ancienne_wave_inf * self.ampvouluz };
        if self.note_encour <= (self.note_min * 100) as f32 {
            out = 0.0;
        }
        out
    }
}

impl Sirene {
    /// a data note with frames to play
    fn has_data(&self, note: i32) -> bool {
        note >= self.note_min
            && (note as usize) < NOMBRE_DE_NOTE
            && self.data.info.get(note as usize).is_some_and(|i| i.data_frames > 0 && i.fdur > 0.0)
    }

    /// The trio's (note, gain) at midicent `mc`: the kernel's weights, the
    /// missing notes' weight given to the others (`Edge`), then `Gains`.
    pub fn trio_gains(&self, mc: i32) -> [(i32, f64); 3] {
        let Some(t) = &self.trio else { return [(-1, 0.0); 3] };
        // the target in anchor steps (semitones when every note is an anchor)
        let s = t.spacing.max(1);
        let p = (mc as f64 / 100.0 - t.origin as f64) / s as f64;
        let (js, mut w) = match t.kernel {
            Kernel::BSpline3 => {
                let c = (p + 0.5).floor();
                let d = p - c;
                let c = c as i32;
                ([c - 1, c, c + 1], [(0.5 - d) * (0.5 - d) / 2.0, 0.75 - d * d, (0.5 + d) * (0.5 + d) / 2.0])
            }
            Kernel::Linear2 => {
                let lo = p.floor();
                let f = p - lo;
                let lo = lo as i32;
                ([lo - 1, lo, lo + 1], [0.0, 1.0 - f, f])
            }
            Kernel::Nearest => {
                let c = (p + 0.5).floor() as i32;
                ([c - 1, c, c + 1], [0.0, 1.0, 0.0])
            }
        };
        let notes = js.map(|j| t.origin + j * s);
        let has = notes.map(|n| self.has_data(n));
        edge_weights(t.edge, &mut w, has);
        let g = |x: f64| match t.gains {
            Gains::Linear => x,
            Gains::Power => x.sqrt(),
        };
        [(notes[0], g(w[0])), (notes[1], g(w[1])), (notes[2], g(w[2]))]
    }

    /// `calculwave()` with the three-note crossfade: three banks (and the one
    /// fading out), each its own note's partials shifted to the target pitch,
    /// their outputs mixed. The
    /// clapet level, the 8-bit hold and the silence under `noteMin` are the
    /// plugin's; the oscillators run in f64 throughout.
    fn calculwave_trio(&mut self) -> f32 {
        if self.ampvouluz < self.ampvoulu {
            self.ampvouluz = (self.ampvouluz as f64 + self.vitesse_clape) as f32;
        }
        if self.ampvouluz > self.ampvoulu {
            self.ampvouluz = (self.ampvouluz as f64 - self.vitesse_clape) as f32;
        }
        self.count8bit = !self.count8bit;

        let mut changed = false;
        let mc = self.midi_cent_voulue;
        if mc != self.trio_mc {
            self.trio_mc = mc;
            let gains = self.trio_gains(mc);
            let wanted = 440.0 * 2f64.powf((mc as f64 / 100.0 - 69.0) / 12.0);
            let octave = self.trio.as_ref().map_or(0, |t| t.octave);
            self.trio_f0 = wanted * 2f64.powi(octave);
            self.trio_dphi = self.trio_f0 * self.deux_pie_sample_rate;
            let hz = |n: i32| match self.trio_hz.get(n as usize) {
                Some(&hz) if n >= 0 => hz,
                _ => self.trio.as_ref().map_or(0.0, |t| note_hz(n, t)),
            };
            let hzs = gains.map(|(n, _)| hz(n));
            let firsts = gains.map(|(n, _)| self.first_frame(n));
            self.trio_notes = gains;
            let (origin, spacing) = self.trio.as_ref().map_or((0, 1), |t| (t.origin, t.spacing.max(1)));
            for b in self.banks.iter_mut() {
                if !gains.iter().any(|&(n, _)| n == b.note) {
                    b.gain = 0.0;
                }
                b.corr_frame = i32::MIN;
            }
            for (k, &(n, g)) in gains.iter().enumerate() {
                // a slot per anchor index (per note when every note is one)
                let b = &mut self.banks[(n - origin).div_euclid(spacing).rem_euclid(4) as usize];
                if b.note != n {
                    b.note = n;
                    b.frame = firsts[k];
                    b.count_k = 0;
                    b.carry = 0.0;
                    b.phase = [0.0; MAX_PARTIEL];
                    b.ampz = [0.0; MAX_PARTIEL];
                    if self.trio_prime {
                        let nn = idx(n);
                        b.frame = self.count_p[nn].max(0);
                        let a = self.data.amp(nn, self.data.play_frame(nn, b.frame as usize, self.skip_silent && matches!(self.frames, FrameControl::Lab(_))));
                        for (z, &a) in b.ampz.iter_mut().zip(a.iter()) {
                            *z = g * a as f64;
                        }
                    }
                    changed = true;
                }
                b.gain = g;
                b.ratio = wanted / hzs[k];
                b.ps = b.ratio * self.deux_pie_sample_rate;
            }
        }

        self.trio_prime = false;
        let compute = self.is16bit || self.count8bit || changed;
        let (sa, sb) = self.smoothing_rate;
        let q = self.qualite as usize;
        let scale = self.trio.as_ref().is_some_and(|t| t.scale_frames);
        let lock = self.trio.as_ref().is_some_and(|t| t.lock);
        let alpha = self.trio.as_ref().map_or(0.0, |t| t.envelope);
        let harmonic_envelope = self.trio.as_ref().is_some_and(|t| t.harmonic_envelope);
        let fast = self.trio.as_ref().is_some_and(|t| t.fast);
        let phasors = fast && self.trio.as_ref().is_some_and(|t| t.f32_phasors && t.lock);
        let mut top = 0usize;
        self.trio_phi += self.trio_dphi;
        if fast && self.trio_phi >= std::f64::consts::TAU {
            // h·Φ moves by whole turns: the same sines, and Φ stays small
            self.trio_phi -= std::f64::consts::TAU;
        }
        let (phi, f0) = (self.trio_phi, self.trio_f0);
        let time_scale = self.time_scale;
        let (data, frames, envelopes, skip) = (&self.data, &self.frames, &self.envelopes, self.skips());
        let mut wave = 0.0f64;
        for b in self.banks.iter_mut() {
            if b.note < 0 {
                continue;
            }
            let n = idx(b.note);
            let clock = Clock { rate: if scale { b.ratio } else { 1.0 }, time_scale };
            b.count_k = advance_frame(data, frames, n, &mut b.frame, b.count_k, clock, &mut b.carry);
            if alpha != 0.0 && b.corr_frame != b.frame {
                b.corr_frame = b.frame;
                let fr = data.freq(n, data.play_frame(n, b.frame.max(0) as usize, skip));
                let env = envelopes.get(n).and_then(|e| e.as_ref());
                for (i, &fi) in fr.iter().enumerate().take(q) {
                    b.corr[i] = match env {
                        Some(e) if fi > 0.0 => {
                            let (from, to) = if lock && harmonic_envelope {
                                // the partial sounds on harmonic h of the target: the
                                // envelope is read there (and h·F/ratio in its note),
                                // not at the slot's own frequency, which jitters
                                let h = (fi as f64 * b.ratio / f0).round().max(1.0);
                                (h * f0 / b.ratio, h * f0)
                            } else {
                                (fi as f64, fi as f64 * b.ratio)
                            };
                            10f64.powf(alpha * (e.at(to) - e.at(from)) / 20.0)
                        }
                        _ => 1.0,
                    };
                }
            }
            let view = FrameView::new(data, frames, skip, n, b.frame, b.count_k, clock, b.carry);
            if fast {
                if lock && !compute {
                    // a locked phase is h·Φ, made afresh when it is read: nothing to keep
                    continue;
                }
                let q = q.min(MAX_PARTIEL);
                let to_h = b.ratio / f0;
                if phasors {
                    // compute is true here: the harmonic numbers and amplitudes, the sines later
                    let gain = b.gain;
                    let fast_read = view.next.is_none() && alpha == 0.0;
                    if fast_read {
                        // straight from the frame: loops the compiler vectorizes
                        for (z, &a) in b.ampz[..q].iter_mut().zip(&view.amp[..q]) {
                            *z = sa * (gain * a as f64) + sb * *z;
                        }
                        for (hp, &f) in b.phase[..q].iter_mut().zip(&view.freq[..q]) {
                            let h = crate::fastsin::round(f as f64 * to_h).max(1.0);
                            *hp = if h < TRIO_HARMONICS as f64 { h } else { 0.0 };
                        }
                    } else {
                        for i in 0..q {
                            let (f, a) = view.at(i);
                            let h = crate::fastsin::round(f as f64 * to_h).max(1.0);
                            b.phase[i] = if h < TRIO_HARMONICS as f64 { h } else { 0.0 };
                            let a = if alpha != 0.0 { a as f64 * b.corr[i] } else { a as f64 };
                            b.ampz[i] = sa * (gain * a) + sb * b.ampz[i];
                        }
                    }
                    // the harmonic numbers sit in `phase` (a locked phase is not kept)
                    let (mut in_order, mut hi) = (true, 0.0f64);
                    for (i, &h) in b.phase[..q].iter().enumerate() {
                        in_order &= h == (i + 1) as f64;
                        hi = hi.max(h);
                    }
                    top = top.max(hi as usize);
                    b.in_order = in_order;
                    continue;
                }
                let (gain, ps) = (b.gain, b.ps);
                for i in 0..q {
                    let (f, a) = view.at(i);
                    b.phase[i] = if lock {
                        crate::fastsin::round(f as f64 * to_h).max(1.0) * phi
                    } else {
                        let p = b.phase[i] + f as f64 * ps;
                        p - crate::fastsin::round(p * (0.5 * std::f64::consts::FRAC_1_PI)) * std::f64::consts::TAU
                    };
                    if compute {
                        let a = if alpha != 0.0 { a as f64 * b.corr[i] } else { a as f64 };
                        b.ampz[i] = sa * (gain * a) + sb * b.ampz[i];
                    }
                }
                if compute {
                    wave += crate::fastsin::sin_dot(&b.phase[..q], &b.ampz[..q]);
                }
                continue;
            }
            for i in 0..q {
                let (f, a) = view.at(i);
                if lock {
                    // the harmonic the partial lands on at the target pitch
                    b.phase[i] = ((f as f64 * b.ratio) / f0).round().max(1.0) * phi;
                } else {
                    b.phase[i] += f as f64 * b.ps;
                }
                if compute {
                    let a = if alpha != 0.0 { a as f64 * b.corr[i] } else { a as f64 };
                    b.ampz[i] = sa * (b.gain * a) + sb * b.ampz[i];
                    wave += b.phase[i].sin() * b.ampz[i];
                }
            }
        }
        if phasors && compute {
            harmonic_sines(phi, &mut self.trio_sines[..(top + 1).min(TRIO_HARMONICS)]);
            self.trio_sines[0] = 0.0;
            let sines = &self.trio_sines;
            for b in self.banks.iter().filter(|b| b.note >= 0) {
                let q = q.min(MAX_PARTIEL);
                if b.in_order && q + 1 <= TRIO_HARMONICS {
                    wave += f32_dot(&sines[1..q + 1], &b.ampz[..q]);
                } else {
                    let mut acc = 0.0f64;
                    for i in 0..q {
                        acc += sines[b.phase[i] as usize] as f64 * b.ampz[i];
                    }
                    wave += acc;
                }
            }
        }
        if compute {
            self.trio_wave = wave;
        }
        let mut out = (self.trio_wave * self.ampvouluz as f64) as f32;
        if self.note_encour <= (self.note_min * 100) as f32 {
            out = 0.0;
        }
        out
    }
}

/// The harmonics `Trio::f32_phasors` tabulates (a partial above sounds nothing)
pub const TRIO_HARMONICS: usize = 512;

/// `out[h] = sin(h·phi)` (f32): cos/sin of `phi` by the kernels' polynomial, then e^{ihφ} for
/// h = 1..8 by products and each next eight from the last eight times e^{i8φ}, eight lanes at
/// a time (vectorized; plain Rust). The error grows with h, about one f32 rounding per step.
pub fn harmonic_sines(phi: f64, out: &mut [f32]) {
    let (_, c, s) = crate::kernels::anchor(phi);
    let (mut zr, mut zi) = ([0.0f32; 8], [0.0f32; 8]);
    zr[0] = c;
    zi[0] = s;
    for k in 1..8 {
        zr[k] = zr[k - 1] * c - zi[k - 1] * s;
        zi[k] = zr[k - 1] * s + zi[k - 1] * c;
    }
    let (c8, s8) = (zr[7], zi[7]);
    if out.is_empty() {
        return;
    }
    let mut h = 1;
    while h < out.len() {
        let n = (out.len() - h).min(8);
        out[h..h + n].copy_from_slice(&zi[..n]);
        h += 8;
        for k in 0..8 {
            let r = zr[k] * c8 - zi[k] * s8;
            zi[k] = zr[k] * s8 + zi[k] * c8;
            zr[k] = r;
        }
    }
}

/// Σ s[i]·a[i], four lanes
#[inline]
fn f32_dot(s: &[f32], a: &[f64]) -> f64 {
    let n = s.len().min(a.len());
    let mut acc = [0.0f64; 4];
    let (sc, ac) = (s[..n].chunks_exact(4), a[..n].chunks_exact(4));
    let (sr, ar) = (sc.remainder(), ac.remainder());
    for (x, y) in sc.zip(ac) {
        for j in 0..4 {
            acc[j] += x[j] as f64 * y[j];
        }
    }
    let mut t = (acc[0] + acc[1]) + (acc[2] + acc[3]);
    for (x, y) in sr.iter().zip(ar) {
        t += *x as f64 * y;
    }
    t
}

/// a data note's pitch in Hz: its nominal pitch, with the trio's tuning
fn note_hz(n: i32, t: &Trio) -> f64 {
    let tune = t.tuning.get(n.max(0) as usize).map_or(1.0, |&c| 2f64.powf(c as f64 / 1200.0));
    440.0 * 2f64.powf((n as f64 - 69.0) / 12.0) * tune
}

/// `Edge`: the weights of the notes without data (`has[k]` false) go to the
/// others, so the total stays what the kernel gave.
pub fn edge_weights(edge: Edge, w: &mut [f64; 3], has: [bool; 3]) {
    match edge {
        Edge::Renormalize => {
            let s: f64 = (0..3).filter(|&k| has[k]).map(|k| w[k]).sum();
            for k in 0..3 {
                w[k] = if has[k] && s > 0.0 { w[k] / s } else { 0.0 };
            }
            // the notes that exist all had weight 0 (1.5 semitones away):
            // the nearest of them takes it all
            if s <= 0.0
                && let Some(k) = [1, 0, 2].into_iter().find(|&k| has[k]) {
                    w[k] = 1.0;
                }
        }
        Edge::Centre => {
            let missing: f64 = (0..3).filter(|&k| !has[k]).map(|k| w[k]).sum();
            for k in 0..3 {
                if !has[k] {
                    w[k] = 0.0;
                }
            }
            let to = if has[1] { Some(1) } else { (0..3).filter(|&k| has[k]).max_by(|&a, &b| w[a].total_cmp(&w[b])) };
            if let Some(k) = to {
                w[k] += missing;
            }
        }
        Edge::Reflect => {
            let orig = *w;
            for (k, mirror) in [(0, 2), (2, 0)] {
                if !has[k] {
                    w[k] = 0.0;
                    if has[mirror] {
                        w[mirror] += orig[k];
                    } else if has[1] {
                        w[1] += orig[k];
                    }
                }
            }
            if !has[1] {
                let m = w[1];
                w[1] = 0.0;
                let ends: Vec<usize> = [0, 2].into_iter().filter(|&k| has[k]).collect();
                for &k in &ends {
                    w[k] += m / ends.len() as f64;
                }
            }
        }
    }
}

/// How long a note's frames last: `fdur` samples at 44.1 kHz, divided by
/// `rate` (a trio bank's ratio, with `Trio::scale_frames`) and times
/// `time_scale` (`Sirene::time_scale`).
#[derive(Clone, Copy)]
struct Clock {
    rate: f64,
    time_scale: f64,
}

impl Clock {
    /// Samples in the current frame (the lab's `speed` too; 1 for the plugin's
    /// frames). At 44.1 kHz (`time_scale` 1) the lengths of the plugin and of
    /// the lab, whole samples; elsewhere the fraction a frame leaves over is
    /// carried to the next (`carry`), so frames keep their length in time.
    #[inline]
    fn len(self, fdur: f32, speed: f32, carry: f64) -> i32 {
        let fdur = scaled(fdur, self.rate);
        if self.time_scale == 1.0 {
            return lab_fdur(fdur, speed);
        }
        let d = fdur as i32;
        if d <= 0 || speed <= 0.0 { 0 } else { (d as f64 / speed as f64 * self.time_scale + carry).floor().max(1.0) as i32 }
    }

    /// `carry` after a frame of `len` samples
    #[inline]
    fn carry(self, fdur: f32, speed: f32, carry: f64, len: i32) -> f64 {
        if self.time_scale == 1.0 {
            return 0.0;
        }
        let d = scaled(fdur, self.rate) as i32;
        carry + d as f64 / speed as f64 * self.time_scale - len as f64
    }
}

/// The frame counter of one note for one more sample, on a note's or a bank's
/// counters (`frame`, its `count_k` and its `carry`).
#[inline]
fn advance_frame(data: &DataSet, frames: &FrameControl, n: usize, frame: &mut i32, count_k: i32, clock: Clock, carry: &mut f64) -> i32 {
    let dure = data.dure.get(n).copied().unwrap_or([0.0; 3]);
    match frames {
        FrameControl::Legacy => {
            let len = Clock { rate: 1.0, ..clock }.len(dure[0], 1.0, *carry);
            if count_k == len {
                *carry = Clock { rate: 1.0, ..clock }.carry(dure[0], 1.0, *carry, len);
                *frame += 1;
                if *frame == dure[1] as i32 {
                    *frame = 0;
                }
                return 1;
            }
            count_k + 1
        }
        FrameControl::Lab(l) => {
            let fdur = clock.len(dure[0], l.speed, *carry);
            if fdur > 0 && count_k >= fdur {
                *carry = clock.carry(dure[0], l.speed, *carry, fdur);
                let s = l.sel.get(n).copied().unwrap_or(FrameSel { start: 0, end: 0, freeze: None });
                *frame = match s.freeze {
                    Some(k) => k as i32,
                    None => {
                        let p = *frame + 1;
                        if p >= s.end as i32 || p < s.start as i32 { s.start as i32 } else { p }
                    }
                };
                return 1;
            }
            count_k + 1
        }
    }
}

/// a frame length in samples divided by `rate`, rounded to f32 as the data is
#[inline]
fn scaled(fdur: f32, rate: f64) -> f32 {
    if rate == 1.0 { fdur } else { (fdur as f64 / rate) as f32 }
}

/// `(freq, amp)` of partial `i` of note `n` at `frame`, between frames when
/// the lab interpolates
#[inline]
#[allow(clippy::too_many_arguments)]
fn partial_at(data: &DataSet, frames: &FrameControl, skip: bool, n: usize, frame: i32, count_k: i32, i: usize, clock: Clock, carry: f64) -> (f32, f32) {
    FrameView::new(data, frames, skip, n, frame, count_k, clock, carry).at(i)
}

/// The frame(s) a note's partials are read from at one sample: `partial_at` for every partial
/// with the lookups done once.
struct FrameView<'a> {
    freq: &'a [f32; MAX_PARTIEL],
    amp: &'a [f32; MAX_PARTIEL],
    /// the next frame and how far towards it, when the lab interpolates
    next: Option<(&'a [f32; MAX_PARTIEL], &'a [f32; MAX_PARTIEL], f32)>,
}

impl<'a> FrameView<'a> {
    #[inline]
    #[allow(clippy::too_many_arguments)]
    fn new(data: &'a DataSet, frames: &FrameControl, skip: bool, n: usize, frame: i32, count_k: i32, clock: Clock, carry: f64) -> Self {
        let p = frame.max(0) as usize;
        let pp = data.play_frame(n, p, skip);
        let mut v = FrameView { freq: data.freq(n, pp), amp: data.amp(n, pp), next: None };
        if let FrameControl::Lab(l) = frames
            && l.interpolate {
                let s = l.sel.get(n).copied().unwrap_or(FrameSel { start: 0, end: 0, freeze: None });
                if s.freeze.is_none() {
                    let fdur = clock.len(data.dure.get(n).map_or(0.0, |d| d[0]), l.speed, carry);
                    if fdur > 0 {
                        let next = if p + 1 >= s.end { s.start } else { p + 1 };
                        // count_k runs 1..=fdur over a frame
                        let t = ((count_k - 1).max(0) as f32) / fdur as f32;
                        let next = data.play_frame(n, next, skip);
                        v.next = Some((data.freq(n, next), data.amp(n, next), t));
                    }
                }
            }
        v
    }

    /// `(freq, amp)` of partial `i`
    #[inline]
    fn at(&self, i: usize) -> (f32, f32) {
        let (f0, a0) = (self.freq[i], self.amp[i]);
        match self.next {
            Some((f1, a1, t)) => (f0 + (f1[i] - f0) * t, a0 + (a1[i] - a0) * t),
            None => (f0, a0),
        }
    }
}

#[inline]
fn idx(note: i32) -> usize {
    (note.max(0) as usize).min(NOTES - 1)
}

/// samples per frame in the lab
#[inline]
pub fn lab_fdur(fdur: f32, speed: f32) -> i32 {
    let d = fdur as i32;
    if d <= 0 || speed <= 0.0 { 0 } else { ((d as f32 / speed).round() as i32).max(1) }
}

/// What `MidiIn` computes goes to the `Sirene` as in `SirenVoiceUnit`
/// (`setnoteFromExt`, `setVelocite`). With the glide off (the lab), a note on
/// jumps to its pitch, and later pitch changes from the timer (vibrato, the
/// pitch wheel, portamento) jump too, but only when the midicent changes, so
/// a held note keeps a pitch set by `jump_to`.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Ctx {
    NoteOn,
    NoteOff,
    Timer,
}

struct ToSiren<'a> {
    siren: &'a mut Sirene,
    glide: bool,
    ctx: Ctx,
    last: &'a mut Option<i32>,
}

impl Sink for ToSiren<'_> {
    fn note(&mut self, note: f32) {
        let n = note as i32;
        if self.glide {
            self.siren.set_note_from_ext(n);
        } else {
            match self.ctx {
                Ctx::NoteOn => {
                    self.siren.set_note_from_ext(n);
                    self.siren.jump_to(n);
                }
                Ctx::NoteOff => self.siren.set_note_from_ext(n),
                Ctx::Timer => {
                    if *self.last != Some(n) {
                        self.siren.jump_to(n);
                    }
                }
            }
        }
        *self.last = Some(n);
    }

    fn volume(&mut self, volume: f32) {
        self.siren.set_velocite(volume as i32);
    }
}

/// `SirenVoiceUnit`: a `MidiIn` driving a `Sirene`, with the plugin's two
/// timers (`timerAudio` every 512 samples, `setnote` every ms). With
/// [`Timing::Time`] their periods are those at 44.1 kHz, in time: 512 and 44
/// samples at 44.1 kHz, fractional elsewhere (a timer fires when its sample
/// count left drops to 0 or below, and gets one period more).
pub struct Voice {
    pub siren: Sirene,
    midi: MidiIn,
    last_note: Option<i32>,
    max_midi_note: i32,
    sample_rate: f64,
    timer_left: f64,
    timer_period: f64,
    set_note_left: f64,
    set_note_period: f64,
    /// the rotor's inertia: `setnote()` slides the pitch. Off, the lab jumps
    /// to the note's pitch at once.
    pub glide: bool,
}

impl Voice {
    pub fn new(def: &SirenDef, data: Arc<DataSet>, vi: &VectorInterval, sample_rate: f64) -> Voice {
        let mut v = Voice {
            siren: Sirene::new(def, data, vi),
            midi: MidiIn::new(def),
            last_note: None,
            max_midi_note: def.max_midi_note,
            sample_rate: DATA_RATE,
            timer_left: 0.0,
            timer_period: 512.0,
            set_note_left: 0.0,
            set_note_period: 44.0,
            glide: true,
        };
        v.set_sample_rate(sample_rate);
        // CC 7 is 0 until the host sends it; the plugin's parameter sends 127
        v.control_change(7, 127);
        v
    }

    pub fn set_sample_rate(&mut self, sr: f64) {
        self.sample_rate = sr;
        self.siren.set_sample_rate(sr);
        let scale = self.siren.time_scale();
        // SirenVoiceUnit: static_cast<int>(sampleRate / 1000.0f), in double
        let plugin_period = |sr: f64| ((sr / 1000.0) as i32).max(1) as f64;
        match self.siren.timing() {
            Timing::Plugin => {
                self.timer_period = 512.0;
                self.set_note_period = plugin_period(sr);
                self.midi.set_sample_rate(sr);
            }
            Timing::Time => {
                self.timer_period = 512.0 * scale;
                self.set_note_period = plugin_period(DATA_RATE) * scale;
                // the vibrato step per timerAudio, which now lasts 512 samples at 44.1 kHz
                self.midi.set_sample_rate(DATA_RATE);
            }
        }
    }

    /// [`Timing`]: how the timers, frames and ramps count time at this rate
    pub fn set_timing(&mut self, timing: Timing) {
        if timing != self.siren.timing() {
            self.siren.set_timing(timing);
            self.set_sample_rate(self.sample_rate);
        }
    }

    pub fn sample_rate(&self) -> f64 {
        self.sample_rate
    }

    fn sink(&mut self, ctx: Ctx) -> (&mut MidiIn, ToSiren<'_>) {
        (&mut self.midi, ToSiren { siren: &mut self.siren, glide: self.glide, ctx, last: &mut self.last_note })
    }

    /// A MIDI note as the plugin receives it: it sounds the data note
    /// `note - 12`.
    pub fn note_on(&mut self, note: i32, velocity: i32) {
        let note = note.min(self.max_midi_note);
        let (midi, mut sink) = self.sink(Ctx::NoteOn);
        midi.start_note(note, velocity, &mut sink);
    }

    pub fn note_off(&mut self, note: i32) {
        let note = note.min(self.max_midi_note);
        let (midi, mut sink) = self.sink(Ctx::NoteOff);
        midi.stop_note(note, &mut sink);
    }

    /// `MidiIn::handleControlChange` (CC 7 volume, 1/9/11 vibrato, 15/92
    /// tremolo, 5 portamento, 16 bend range, 72/73 release/attack, 121 reset)
    pub fn control_change(&mut self, cc: i32, value: i32) {
        let (midi, mut sink) = self.sink(Ctx::Timer);
        midi.control_change(cc, value, &mut sink);
    }

    /// `MidiIn::handlePitchWheel`: 14 bits, 8192 = centre; the range is CC 16
    pub fn pitch_wheel(&mut self, lsb: i32, msb: i32) {
        self.midi.pitch_wheel(lsb, msb);
    }

    pub fn midi(&self) -> &MidiIn {
        &self.midi
    }

    pub fn process(&mut self) -> f32 {
        if self.timer_left <= 0.0 {
            let (midi, mut sink) = self.sink(Ctx::Timer);
            midi.timer_audio(&mut sink);
            self.timer_left += self.timer_period;
        }
        self.timer_left -= 1.0;
        if self.set_note_left <= 0.0 {
            if self.glide {
                self.siren.setnote();
            } else if matches!(self.siren.frames, FrameControl::Legacy) {
                // the plugin's millisecond reset of the frame counters
                let mc = self.siren.midicent();
                self.siren.set_midicent(mc);
            }
            self.set_note_left += self.set_note_period;
        }
        self.set_note_left -= 1.0;
        self.siren.calculwave()
    }
}

/// What a held note carries from one synthesis to another (the plugin's DSP swap with a
/// note held, `csr_siren_get_handoff`): the rotor's slide, the flap's level, each partial's
/// smoothed amplitude and phase, and the frames the two notes are on. The layout is the C
/// ABI's `CsrHandoff` and the C++ `Sirene::Handoff`.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Handoff {
    pub note_voulue_avant_slide: i32,
    pub note_encour: f32,
    pub inter_depart: i32,
    pub ampvoulu: f32,
    pub ampvouluz: f32,
    pub count_k_inf: i32,
    pub count_k_sup: i32,
    pub count_p_inf: i32,
    pub count_p_sup: i32,
    pub ampz: [f32; MAX_PARTIEL],
    pub amp: [f32; MAX_PARTIEL],
    pub phase_inf: [f64; MAX_PARTIEL],
    pub phase_sup: [f64; MAX_PARTIEL],
    pub ancienne_wave_inf: f32,
    /// the trio's harmonic phase (its locked partials are h·Φ); 0 elsewhere
    pub trio_phi: f64,
    /// read only: 1 when this voice is a trio (its partials don't keep the others' phases)
    pub trio: i32,
}

impl Sirene {
    /// `skip_silent` where it applies: the lab's frames. The plugin's frames (`Legacy`) stay the
    /// plugin's, bit for bit: their counters restart at every `setMidicent()` and seldom leave
    /// the first frames.
    #[inline]
    pub fn skips(&self) -> bool {
        self.skip_silent && matches!(self.frames, FrameControl::Lab(_))
    }
}

impl Sirene {
    /// The state a swap carries over. Changes nothing. Real-time.
    pub fn handoff(&self) -> Handoff {
        Handoff {
            note_voulue_avant_slide: self.note_voulue_avant_slide,
            note_encour: self.note_encour,
            inter_depart: self.inter_depart,
            ampvoulu: self.ampvoulu,
            ampvouluz: self.ampvouluz,
            count_k_inf: self.count_k_inf,
            count_k_sup: self.count_k_sup,
            count_p_inf: self.count_p[idx(self.note_inf)],
            count_p_sup: self.count_p[idx(self.note_sup)],
            ampz: self.ampz,
            amp: self.amp,
            phase_inf: self.phase_inf,
            phase_sup: self.phase_inf,
            ancienne_wave_inf: self.ancienne_wave_inf,
            trio_phi: self.trio_phi,
            trio: self.trio.is_some() as i32,
        }
    }

    /// Takes over a held note: as if this voice had played it (`set_midicent` on the
    /// carried pitch, then the counters, levels and phases). Real-time: no allocation.
    pub fn set_handoff(&mut self, h: &Handoff) {
        self.note_voulue_avant_slide = h.note_voulue_avant_slide.min(self.note_midi_cent_max);
        self.note_encour = h.note_encour;
        self.inter_depart = h.inter_depart;
        self.set_midicent(self.note_encour as i32);
        self.ampvoulu = h.ampvoulu;
        self.ampvouluz = h.ampvouluz;
        self.count_k_inf = h.count_k_inf;
        self.count_k_sup = h.count_k_sup;
        self.count_p[idx(self.note_inf)] = h.count_p_inf;
        self.count_p[idx(self.note_sup)] = h.count_p_sup;
        self.frame_carry[idx(self.note_inf)] = 0.0;
        self.frame_carry[idx(self.note_sup)] = 0.0;
        self.ampz = h.ampz;
        self.amp = h.amp;
        self.phase_inf = h.phase_inf;
        self.ancienne_wave_inf = h.ancienne_wave_inf;
        self.ancien_note_voulue = self.note_inf;
        if self.trio.is_some() {
            // the banks are set up at the next sample, on the carried frames and level
            self.trio_mc = i32::MIN;
            self.trio_prime = true;
            self.trio_phi = h.trio_phi.rem_euclid(std::f64::consts::TAU);
        }
    }
}
