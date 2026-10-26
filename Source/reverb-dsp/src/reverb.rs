//! Port of `Source/ComposeSirenCore/Reverb.h` and `Reverb.cpp`.
//!
//! `Reverb` wraps [`Mareverbe`], pre-reverb high-pass (low cut) and post-reverb low-pass (high cut)
//! 2nd-order Butterworth filters ([`IirFilter`]), and parameter ramps ([`Rampe`]).

use crate::biquad::{IirCoefficients, IirFilter};
use crate::mareverbe::{Mareverbe, fma_f32};
use crate::rampe::Rampe;

/// Operating mode for [`Reverb`].
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum ReverbMode {
    /// Bit-for-bit identical to the C++ `Reverb` in `Source/ComposeSirenCore/Reverb.cpp`:
    /// - Both L and R channels pass through `reverbHighpassL` before `Mareverbe`.
    /// - Parameter ramps use `rampe<float>::setTarget` as in `rampe.h` and step once per block.
    /// - In-place calls (`process_in_place` / `csr_reverb_process` with `in == out`) follow the
    ///   C++ pointer-aliasing behavior where `outL[i]` overwrites `inL[i]` before the dry/wet mix.
    #[default]
    Plugin,
    /// Independent stereo high-pass filtering (`reverbHighpassL` on L, `reverbHighpassR` on R),
    /// sample-rate-scaled 10 ms ramps stepped per sample, and non-aliasing dry/wet mix on
    /// in-place buffers.
    StereoTime,
}

/// ComposeSiren's stereo reverb unit (`Reverb` in `Source/ComposeSirenCore/Reverb.h`).
#[derive(Clone, Debug)]
pub struct Reverb {
    rvb: Mareverbe,
    enabled: bool,
    en: bool,
    dry_wet: f32,
    rdw: Rampe,
    damping: f32,
    rd: Rampe,
    room_size: f32,
    rrs: Rampe,
    width: f32,
    rw: Rampe,
    low_cut: f32,
    high_cut: f32,
    dry: f32,
    wet: f32,
    sample_rate: f64,
    high_pass_freq: f32,
    reverb_highpass_l: IirFilter,
    reverb_highpass_r: IirFilter,
    low_pass_freq: f32,
    reverb_lowpass_l: IirFilter,
    reverb_lowpass_r: IirFilter,
    mode: ReverbMode,
}

impl Default for Reverb {
    fn default() -> Self {
        Self::new()
    }
}

impl Reverb {
    /// Creates a `Reverb` in the exact state produced by `Reverb::Reverb()` in C++.
    pub fn new() -> Self {
        let mut rvb = Mareverbe::new();
        rvb.set_dry(0.0);
        rvb.set_wet(1.0);
        Self {
            rvb,
            enabled: true,
            en: true,
            dry_wet: 0.0,
            rdw: Rampe::new(0.0, 10.0),
            damping: 0.0,
            rd: Rampe::new(0.0, 10.0),
            room_size: 0.0,
            rrs: Rampe::new(0.0, 10.0),
            width: 0.0,
            rw: Rampe::new(0.0, 10.0),
            low_cut: 0.0,
            high_cut: 0.0,
            dry: 0.0,
            wet: 1.0,
            sample_rate: 44100.0,
            high_pass_freq: 1000.0,
            reverb_highpass_l: IirFilter::new(),
            reverb_highpass_r: IirFilter::new(),
            low_pass_freq: 10000.0,
            reverb_lowpass_l: IirFilter::new(),
            reverb_lowpass_r: IirFilter::new(),
            mode: ReverbMode::Plugin,
        }
    }

    /// Creates a `Reverb` initialized with SirenOrchestra's default parameter values
    /// (`enable = true`, `dry_wet = 0.0`, `damping = 0.5`, `room_size = 0.5`, `width = 1.0`,
    /// `low_cut = 100.0` Hz, `high_cut = 8000.0` Hz) and prepared at `sample_rate`.
    pub fn with_plugin_defaults(sample_rate: f64) -> Self {
        let mut r = Self::new();
        r.enable(true);
        r.set_dry_wet(0.0);
        r.set_damping(0.5);
        r.set_room_size(0.5);
        r.set_width(1.0);
        r.set_low_cut(100.0);
        r.set_high_cut(8000.0);
        r.set_sample_rate(sample_rate);
        r
    }

    pub fn mode(&self) -> ReverbMode {
        self.mode
    }

    pub fn set_mode(&mut self, mode: ReverbMode) {
        self.mode = mode;
        if mode == ReverbMode::StereoTime {
            self.rdw.set_sample_rate(self.sample_rate);
            self.rd.set_sample_rate(self.sample_rate);
            self.rrs.set_sample_rate(self.sample_rate);
            self.rw.set_sample_rate(self.sample_rate);
        }
    }

    /// Port of `Reverb::setSampleRate`.
    pub fn set_sample_rate(&mut self, sr: f64) {
        self.sample_rate = sr;
        if self.mode == ReverbMode::StereoTime {
            self.rdw.set_sample_rate(sr);
            self.rd.set_sample_rate(sr);
            self.rrs.set_sample_rate(sr);
            self.rw.set_sample_rate(sr);
        }
        self.update_high_pass_coefficients();
        self.update_low_pass_coefficients();
    }

    pub fn sample_rate(&self) -> f64 {
        self.sample_rate
    }

    /// Port of `Reverb::enable`.
    pub fn enable(&mut self, e: bool) {
        self.enabled = e;
    }

    pub fn is_enabled(&self) -> bool {
        self.enabled
    }

    /// Port of `Reverb::setDryWet`.
    pub fn set_dry_wet(&mut self, d: f32) {
        self.dry_wet = d;
    }

    pub fn dry_wet(&self) -> f32 {
        self.dry_wet
    }

    /// Port of `Reverb::setDamping`.
    pub fn set_damping(&mut self, d: f32) {
        self.damping = d;
    }

    pub fn damping(&self) -> f32 {
        self.damping
    }

    /// Port of `Reverb::setRoomSize`.
    pub fn set_room_size(&mut self, s: f32) {
        self.room_size = s;
    }

    pub fn room_size(&self) -> f32 {
        self.room_size
    }

    /// Port of `Reverb::setWidth`.
    pub fn set_width(&mut self, w: f32) {
        self.width = w;
    }

    pub fn width(&self) -> f32 {
        self.width
    }

    /// Port of `Reverb::setLowCut` (sets the high-pass filter cutoff frequency in Hz).
    pub fn set_low_cut(&mut self, l: f32) {
        self.low_cut = l;
    }

    pub fn low_cut(&self) -> f32 {
        self.low_cut
    }

    /// Port of `Reverb::setHighCut` (sets the low-pass filter cutoff frequency in Hz).
    pub fn set_high_cut(&mut self, h: f32) {
        self.high_cut = h;
    }

    pub fn high_cut(&self) -> f32 {
        self.high_cut
    }

    /// Port of `Reverb::beginProcessBlock`.
    pub fn begin_process_block(&mut self) {
        self.en = self.enabled;
        match self.mode {
            ReverbMode::Plugin => {
                self.rdw.set_target(self.dry_wet);
                self.rd.set_target(self.damping);
                self.rrs.set_target(self.room_size);
                self.rw.set_target(self.width);
            }
            ReverbMode::StereoTime => {
                self.rdw.set_target_time(self.dry_wet);
                self.rd.set_target_time(self.damping);
                self.rrs.set_target_time(self.room_size);
                self.rw.set_target_time(self.width);
            }
        }

        let v = self.low_cut;
        if v != self.high_pass_freq {
            self.high_pass_freq = v;
            self.update_high_pass_coefficients();
        }
        let v = self.high_cut;
        if v != self.low_pass_freq {
            self.low_pass_freq = v;
            self.update_low_pass_coefficients();
        }
    }

    fn update_high_pass_coefficients(&mut self) {
        let coeffs = IirCoefficients::make_high_pass(self.sample_rate, self.high_pass_freq as f64);
        self.reverb_highpass_l.set_coefficients(coeffs);
        self.reverb_highpass_r.set_coefficients(coeffs);
    }

    fn update_low_pass_coefficients(&mut self) {
        let coeffs = IirCoefficients::make_low_pass(self.sample_rate, self.low_pass_freq as f64);
        self.reverb_lowpass_l.set_coefficients(coeffs);
        self.reverb_lowpass_r.set_coefficients(coeffs);
    }

    /// Clears the internal comb, allpass, and biquad filter delay lines.
    pub fn mute(&mut self) {
        self.rvb.mute();
        self.reverb_lowpass_l.reset();
        self.reverb_lowpass_r.reset();
        self.reverb_highpass_l.reset();
        self.reverb_highpass_r.reset();
    }

    /// Access to the inner [`Mareverbe`] engine.
    pub fn mareverbe(&self) -> &Mareverbe {
        &self.rvb
    }

    /// Mutable access to the inner [`Mareverbe`] engine.
    pub fn mareverbe_mut(&mut self) -> &mut Mareverbe {
        &mut self.rvb
    }

    #[inline(always)]
    fn step_ramps(&mut self) {
        let mut v = 0.0_f32;
        if self.rdw.process(&mut v) {
            self.dry = 1.0 - v;
            self.wet = v;
        }
        if self.rd.process(&mut v) {
            self.rvb.set_damp(v);
        }
        if self.rrs.process(&mut v) {
            self.rvb.set_room_size(v);
        }
        if self.rw.process(&mut v) {
            self.rvb.set_width(v);
        }
    }

    #[inline(always)]
    fn step_ramps_time(&mut self) {
        let mut v = 0.0_f32;
        if self.rdw.process_time(&mut v) {
            self.dry = 1.0 - v;
            self.wet = v;
        }
        if self.rd.process_time(&mut v) {
            self.rvb.set_damp(v);
        }
        if self.rrs.process_time(&mut v) {
            self.rvb.set_room_size(v);
        }
        if self.rw.process_time(&mut v) {
            self.rvb.set_width(v);
        }
    }

    #[inline(always)]
    fn filter_and_reverb(&mut self, in_l: f32, in_r: f32) -> (f32, f32) {
        // filter out low frequencies in dry signal before reverberating
        let hp_l = self.reverb_highpass_l.process_single_sample_raw(in_l);
        let hp_r = match self.mode {
            // Reverb.cpp line 88 calls reverbHighpassL on both channels.
            ReverbMode::Plugin => self.reverb_highpass_l.process_single_sample_raw(in_r),
            ReverbMode::StereoTime => self.reverb_highpass_r.process_single_sample_raw(in_r),
        };
        // reverberate (full wet signal out)
        let (wet_l, wet_r) = self.rvb.process_sample(hp_l, hp_r);
        // filter out high frequencies in reverberated signal
        let lp_l = self.reverb_lowpass_l.process_single_sample_raw(wet_l);
        let lp_r = self.reverb_lowpass_r.process_single_sample_raw(wet_r);
        (lp_l, lp_r)
    }

    /// Port of `Reverb::process` for separate input and output buffers (`in != out`).
    pub fn process(&mut self, in_l: &[f32], in_r: &[f32], out_l: &mut [f32], out_r: &mut [f32]) {
        let n = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        if self.en {
            match self.mode {
                ReverbMode::Plugin => {
                    self.step_ramps();
                    for i in 0..n {
                        let (wet_l, wet_r) = self.filter_and_reverb(in_l[i], in_r[i]);
                        out_l[i] = fma_f32(in_l[i], self.dry, wet_l * self.wet);
                        out_r[i] = fma_f32(in_r[i], self.dry, wet_r * self.wet);
                    }
                }
                ReverbMode::StereoTime => {
                    for i in 0..n {
                        self.step_ramps_time();
                        let (wet_l, wet_r) = self.filter_and_reverb(in_l[i], in_r[i]);
                        out_l[i] = fma_f32(in_l[i], self.dry, wet_l * self.wet);
                        out_r[i] = fma_f32(in_r[i], self.dry, wet_r * self.wet);
                    }
                }
            }
        } else {
            out_l[..n].copy_from_slice(&in_l[..n]);
            out_r[..n].copy_from_slice(&in_r[..n]);
        }
    }

    /// Port of `Reverb::process` when called in-place (`reverb.process(bufL, bufR, bufL, bufR, n)`,
    /// as `SirenOrchestraPluginProcessor::processBlock` does).
    ///
    /// In [`ReverbMode::Plugin`], `outL[i]` overwrites `inL[i]` before the final mix line
    /// `outL[i] = inL[i] * dry + outL[i] * wet`, matching the C++ pointer-aliasing behavior bit for bit.
    /// In [`ReverbMode::StereoTime`], the dry input sample is preserved before mixing.
    pub fn process_in_place(&mut self, buf_l: &mut [f32], buf_r: &mut [f32]) {
        let n = buf_l.len().min(buf_r.len());
        if self.en {
            match self.mode {
                ReverbMode::Plugin => {
                    self.step_ramps();
                    for i in 0..n {
                        let (wet_l, wet_r) = self.filter_and_reverb(buf_l[i], buf_r[i]);
                        buf_l[i] = wet_l;
                        buf_r[i] = wet_r;
                        buf_l[i] = fma_f32(buf_l[i], self.dry, buf_l[i] * self.wet);
                        buf_r[i] = fma_f32(buf_r[i], self.dry, buf_r[i] * self.wet);
                    }
                }
                ReverbMode::StereoTime => {
                    for i in 0..n {
                        self.step_ramps_time();
                        let dry_l = buf_l[i];
                        let dry_r = buf_r[i];
                        let (wet_l, wet_r) = self.filter_and_reverb(dry_l, dry_r);
                        buf_l[i] = fma_f32(dry_l, self.dry, wet_l * self.wet);
                        buf_r[i] = fma_f32(dry_r, self.dry, wet_r * self.wet);
                    }
                }
            }
        }
    }
}
