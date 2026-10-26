//! Port of `juce::IIRCoefficients` and `juce::IIRFilter` (`juce_IIRFilter.h` / `juce_IIRFilter.cpp`),
//! used by `Reverb` for pre-reverb high-pass (low cut) and low-pass (high cut) filtering.

use crate::mareverbe::fma_f32;

#[inline(always)]
fn fma_f64(a: f64, b: f64, c: f64) -> f64 {
    #[cfg(target_arch = "aarch64")]
    {
        a.mul_add(b, c)
    }
    #[cfg(not(target_arch = "aarch64"))]
    {
        a * b + c
    }
}

/// Normalized 2nd-order IIR filter coefficients (`[b0, b1, b2, a1, a2]`), matching
/// `juce::IIRCoefficients`.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct IirCoefficients {
    pub coefficients: [f32; 5],
}

impl IirCoefficients {
    pub fn new(c1: f64, c2: f64, c3: f64, c4: f64, c5: f64, c6: f64) -> Self {
        let a = 1.0 / c4;
        Self {
            coefficients: [
                (c1 * a) as f32,
                (c2 * a) as f32,
                (c3 * a) as f32,
                (c5 * a) as f32,
                (c6 * a) as f32,
            ],
        }
    }

    /// 2nd-order Butterworth low-pass (`Q = 1 / sqrt(2)`), matching `juce::IIRCoefficients::makeLowPass`.
    pub fn make_low_pass(sample_rate: f64, cutoff_frequency: f64) -> Self {
        Self::make_low_pass_with_q(
            sample_rate,
            cutoff_frequency,
            1.0 / std::f64::consts::SQRT_2,
        )
    }

    /// 2nd-order low-pass with custom `Q`, matching `juce::IIRCoefficients::makeLowPass`.
    pub fn make_low_pass_with_q(sample_rate: f64, cutoff_frequency: f64, q: f64) -> Self {
        let n = 1.0 / (std::f64::consts::PI * cutoff_frequency / sample_rate).tan();
        let n2 = n * n;
        let inv_q = 1.0 / q;
        let c1 = 1.0 / (fma_f64(inv_q, n, 1.0) + n2);

        Self::new(
            c1,
            c1 * 2.0,
            c1,
            1.0,
            c1 * 2.0 * (1.0 - n2),
            c1 * (fma_f64(-inv_q, n, 1.0) + n2),
        )
    }

    /// 2nd-order Butterworth high-pass (`Q = 1 / sqrt(2)`), matching `juce::IIRCoefficients::makeHighPass`.
    pub fn make_high_pass(sample_rate: f64, cutoff_frequency: f64) -> Self {
        Self::make_high_pass_with_q(sample_rate, cutoff_frequency, 1.0 / 2.0_f64.sqrt())
    }

    /// 2nd-order high-pass with custom `Q`, matching `juce::IIRCoefficients::makeHighPass`.
    pub fn make_high_pass_with_q(sample_rate: f64, cutoff_frequency: f64, q: f64) -> Self {
        let n = (std::f64::consts::PI * cutoff_frequency / sample_rate).tan();
        let n2 = n * n;
        let inv_q = 1.0 / q;
        let c1 = 1.0 / (fma_f64(inv_q, n, 1.0) + n2);

        Self::new(
            c1,
            c1 * -2.0,
            c1,
            1.0,
            c1 * 2.0 * (n2 - 1.0),
            c1 * (fma_f64(-inv_q, n, 1.0) + n2),
        )
    }
}

/// Transposed Direct Form II biquad filter matching `juce::IIRFilter`.
#[derive(Clone, Debug, Default)]
pub struct IirFilter {
    coefficients: IirCoefficients,
    v1: f32,
    v2: f32,
    active: bool,
}

impl IirFilter {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn reset(&mut self) {
        self.v1 = 0.0;
        self.v2 = 0.0;
    }

    pub fn set_coefficients(&mut self, new_coefficients: IirCoefficients) {
        self.coefficients = new_coefficients;
        self.active = true;
    }

    pub fn make_inactive(&mut self) {
        self.active = false;
    }

    pub fn is_active(&self) -> bool {
        self.active
    }

    pub fn coefficients(&self) -> IirCoefficients {
        self.coefficients
    }

    /// Port of `juce::IIRFilter::processSingleSampleRaw`.
    #[inline(always)]
    pub fn process_single_sample_raw(&mut self, in_sample: f32) -> f32 {
        let c = &self.coefficients.coefficients;
        #[allow(unused_mut)]
        let mut out = fma_f32(c[0], in_sample, self.v1);

        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        {
            if !(out < -1.0e-8 || out > 1.0e-8) {
                out = 0.0;
            }
        }

        self.v1 = fma_f32(c[1], in_sample, -c[3] * out) + self.v2;
        self.v2 = fma_f32(c[2], in_sample, -c[4] * out);

        out
    }
}
