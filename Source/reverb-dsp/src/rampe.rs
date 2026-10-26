//! Port of `Source/ComposeSirenCore/lib/dsp/rampe.h` (`rampe<float>`).

/// Linear parameter ramp (`rampe<float>` in `rampe.h`).
#[derive(Clone, Debug)]
pub struct Rampe {
    ramping: bool,
    target: f32,
    duration: f32,
    increment: f32,
    current: f32,
    sample_rate: f64,
}

impl Default for Rampe {
    fn default() -> Self {
        Self::new(0.0, 10.0)
    }
}

impl Rampe {
    pub const EPSILON: f32 = 0.001;

    pub fn new(default_value: f32, default_duration: f32) -> Self {
        Self {
            ramping: false,
            target: f32::NAN,
            duration: default_duration,
            increment: 1.0,
            current: default_value,
            sample_rate: 44100.0,
        }
    }

    pub fn set_sample_rate(&mut self, sr: f64) {
        if self.ramping {
            self.increment *= (self.sample_rate / sr) as f32;
        }
        self.sample_rate = sr;
    }

    pub fn sample_rate(&self) -> f64 {
        self.sample_rate
    }

    pub fn set_duration(&mut self, d: f32) {
        self.duration = d.max(Self::EPSILON);
    }

    pub fn duration(&self) -> f32 {
        self.duration
    }

    /// Port of `rampe<float>::setTarget` in `rampe.h` (plugin-exact).
    pub fn set_target(&mut self, t: f32) {
        self.target = t;
        let diff = (t - self.current) as f64;
        let denom = 1000.0 * (self.duration as f64) / self.sample_rate;
        self.increment = (diff / denom) as f32;
        self.ramping = true;
    }

    /// Time-accurate ramp to `t` over `duration` milliseconds (`sample_rate * duration / 1000.0` samples).
    pub fn set_target_time(&mut self, t: f32) {
        if t == self.target {
            return;
        }
        self.target = t;
        let samples = self.sample_rate * (self.duration as f64) / 1000.0;
        if samples <= 1.0 || (t - self.current).abs() == 0.0 {
            self.current = t;
            self.increment = 0.0;
            self.ramping = true;
        } else {
            self.increment = ((t - self.current) as f64 / samples) as f32;
            self.ramping = true;
        }
    }

    pub fn set_current(&mut self, v: f32) {
        self.current = v;
        self.ramping = false;
    }

    pub fn value(&self) -> f32 {
        self.current
    }

    pub fn target(&self) -> f32 {
        self.target
    }

    pub fn is_ramping(&self) -> bool {
        self.ramping
    }

    /// Port of `rampe<float>::process` in `rampe.h`.
    #[inline(always)]
    pub fn process(&mut self, v: &mut f32) -> bool {
        if self.ramping {
            self.current += self.increment;
            if (self.increment < 0.0 && self.current <= self.target)
                || (self.increment > 0.0 && self.current >= self.target)
            {
                self.ramping = false;
                self.current = self.target;
            }
            *v = self.current;
            return true;
        }
        *v = self.current;
        false
    }

    /// Per-sample ramp step for `ReverbMode::StereoTime`: also finishes immediately when `increment == 0.0`.
    #[inline(always)]
    pub fn process_time(&mut self, v: &mut f32) -> bool {
        if self.ramping {
            self.current += self.increment;
            if self.increment == 0.0
                || (self.increment < 0.0 && self.current <= self.target)
                || (self.increment > 0.0 && self.current >= self.target)
            {
                self.ramping = false;
                self.current = self.target;
            }
            *v = self.current;
            return true;
        }
        *v = self.current;
        false
    }
}
