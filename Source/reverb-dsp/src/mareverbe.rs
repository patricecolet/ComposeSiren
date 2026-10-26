//! Port of `Source/ComposeSirenCore/lib/dsp/mareverbe.cpp`, `comb.cpp` and `allpass.cpp`
//! (Jezar at Dreampoint's Freeverb, 8 parallel feedback comb filters and 4 series allpass
//! filters per channel, with stereo spread 23).

pub const NUM_COMBS: usize = 8;
pub const NUM_ALLPASSES: usize = 4;
pub const MUTED: f32 = 0.0;
pub const FIXED_GAIN: f32 = 0.015;
pub const SCALE_WET: f32 = 3.0;
pub const SCALE_DRY: f32 = 2.0;
pub const SCALE_DAMP: f32 = 0.4;
pub const SCALE_ROOM: f32 = 0.28;
pub const OFFSET_ROOM: f32 = 0.7;
pub const INITIAL_ROOM: f32 = 0.5;
pub const INITIAL_DAMP: f32 = 0.5;
pub const INITIAL_WET: f32 = 1.0 / SCALE_WET;
pub const INITIAL_DRY: f32 = 0.0;
pub const INITIAL_WIDTH: f32 = 1.0;
pub const INITIAL_MODE: f32 = 0.0;
pub const FREEZE_MODE: f32 = 0.5;
pub const STEREO_SPREAD: usize = 23;

pub const COMB_TUNING_L: [usize; NUM_COMBS] = [1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617];
pub const COMB_TUNING_R: [usize; NUM_COMBS] = [
    1116 + STEREO_SPREAD,
    1188 + STEREO_SPREAD,
    1277 + STEREO_SPREAD,
    1356 + STEREO_SPREAD,
    1422 + STEREO_SPREAD,
    1491 + STEREO_SPREAD,
    1557 + STEREO_SPREAD,
    1617 + STEREO_SPREAD,
];

pub const ALLPASS_TUNING_L: [usize; NUM_ALLPASSES] = [556, 441, 341, 225];
pub const ALLPASS_TUNING_R: [usize; NUM_ALLPASSES] = [
    556 + STEREO_SPREAD,
    441 + STEREO_SPREAD,
    341 + STEREO_SPREAD,
    225 + STEREO_SPREAD,
];

#[inline(always)]
pub(crate) fn fma_f32(a: f32, b: f32, c: f32) -> f32 {
    #[cfg(target_arch = "aarch64")]
    {
        a.mul_add(b, c)
    }
    #[cfg(not(target_arch = "aarch64"))]
    {
        a * b + c
    }
}

/// Feedback comb filter with one-pole lowpass damping in the feedback loop (`comb.h` / `comb.cpp`).
#[derive(Clone, Debug)]
pub struct Comb {
    feedback: f32,
    filterstore: f32,
    damp1: f32,
    damp2: f32,
    buffer: Vec<f32>,
    bufidx: usize,
}

impl Comb {
    pub fn new(size: usize) -> Self {
        Self {
            feedback: 0.0,
            filterstore: 0.0,
            damp1: 0.0,
            damp2: 0.0,
            buffer: vec![0.0; size],
            bufidx: 0,
        }
    }

    #[inline(always)]
    pub fn process(&mut self, input: f32) -> f32 {
        let output = self.buffer[self.bufidx];
        if !((self.filterstore <= 1.0) && (self.filterstore >= -1.0)) {
            self.filterstore = 0.0;
        }
        self.filterstore = fma_f32(output, self.damp2, self.filterstore * self.damp1);
        self.buffer[self.bufidx] = fma_f32(self.filterstore, self.feedback, input);
        self.bufidx += 1;
        if self.bufidx >= self.buffer.len() {
            self.bufidx = 0;
        }
        output
    }

    pub fn mute(&mut self) {
        self.buffer.fill(0.0);
    }

    pub fn set_damp(&mut self, val: f32) {
        self.damp1 = val;
        self.damp2 = 1.0 - val;
    }

    pub fn damp(&self) -> f32 {
        self.damp1
    }

    pub fn set_feedback(&mut self, val: f32) {
        self.feedback = val;
    }

    pub fn feedback(&self) -> f32 {
        self.feedback
    }
}

/// Schroeder allpass filter (`allpass.h` / `allpass.cpp`).
#[derive(Clone, Debug)]
pub struct Allpass {
    feedback: f32,
    buffer: Vec<f32>,
    bufidx: usize,
}

impl Allpass {
    pub fn new(size: usize) -> Self {
        Self {
            feedback: 0.0,
            buffer: vec![0.0; size],
            bufidx: 0,
        }
    }

    #[inline(always)]
    pub fn process(&mut self, input: f32) -> f32 {
        let bufout = self.buffer[self.bufidx];
        let output = -input + bufout;
        self.buffer[self.bufidx] = fma_f32(bufout, self.feedback, input);
        self.bufidx += 1;
        if self.bufidx >= self.buffer.len() {
            self.bufidx = 0;
        }
        output
    }

    pub fn mute(&mut self) {
        self.buffer.fill(0.0);
    }

    pub fn set_feedback(&mut self, val: f32) {
        self.feedback = val;
    }

    pub fn feedback(&self) -> f32 {
        self.feedback
    }
}

/// Hard-clips each sample of `buf` to `[-s, s]` (`dsp_clip` in `mareverbe.cpp`).
#[inline]
pub fn dsp_clip(buf: &mut [f32], s: f32) {
    for b in buf {
        if *b > s {
            *b = s;
        }
        if *b < -s {
            *b = -s;
        }
    }
}

#[inline(always)]
fn clip_sample(mut x: f32, s: f32) -> f32 {
    if x > s {
        x = s;
    }
    if x < -s {
        x = -s;
    }
    x
}

/// Stereo Freeverb engine (`mareverbe` in `mareverbe.h` / `mareverbe.cpp`).
#[derive(Clone, Debug)]
pub struct Mareverbe {
    gain: f32,
    roomsize: f32,
    roomsize1: f32,
    damp: f32,
    damp1: f32,
    wet: f32,
    wet1: f32,
    wet2: f32,
    dry: f32,
    width: f32,
    mode: f32,
    comb_l: [Comb; NUM_COMBS],
    comb_r: [Comb; NUM_COMBS],
    allpass_l: [Allpass; NUM_ALLPASSES],
    allpass_r: [Allpass; NUM_ALLPASSES],
}

impl Default for Mareverbe {
    fn default() -> Self {
        Self::new()
    }
}

impl Mareverbe {
    pub fn new() -> Self {
        let comb_l = std::array::from_fn(|i| Comb::new(COMB_TUNING_L[i]));
        let comb_r = std::array::from_fn(|i| Comb::new(COMB_TUNING_R[i]));
        let mut allpass_l = std::array::from_fn(|i| Allpass::new(ALLPASS_TUNING_L[i]));
        let mut allpass_r = std::array::from_fn(|i| Allpass::new(ALLPASS_TUNING_R[i]));
        for i in 0..NUM_ALLPASSES {
            allpass_l[i].set_feedback(0.5);
            allpass_r[i].set_feedback(0.5);
        }

        let mut m = Self {
            gain: 0.0,
            roomsize: 0.0,
            roomsize1: 0.0,
            damp: 0.0,
            damp1: 0.0,
            wet: 0.0,
            wet1: 0.0,
            wet2: 0.0,
            dry: 0.0,
            width: 0.0,
            mode: 0.0,
            comb_l,
            comb_r,
            allpass_l,
            allpass_r,
        };

        m.set_wet(INITIAL_WET);
        m.set_room_size(INITIAL_ROOM);
        m.set_dry(INITIAL_DRY);
        m.set_damp(INITIAL_DAMP);
        m.set_width(INITIAL_WIDTH);
        m.set_mode(INITIAL_MODE);
        m.mute();
        m
    }

    pub fn mute(&mut self) {
        if self.mode() >= FREEZE_MODE {
            return;
        }
        for i in 0..NUM_COMBS {
            self.comb_l[i].mute();
            self.comb_r[i].mute();
        }
        for i in 0..NUM_ALLPASSES {
            self.allpass_l[i].mute();
            self.allpass_r[i].mute();
        }
    }

    /// Processes a single stereo sample without the final `dsp_clip(..., 1.0)`.
    #[inline(always)]
    pub fn process_sample_unclipped(&mut self, in_l: f32, in_r: f32) -> (f32, f32) {
        let mut out_l = 0.0_f32;
        let mut out_r = 0.0_f32;
        let proxy_l = in_l * self.gain;
        let proxy_r = in_r * self.gain;

        for i in 0..NUM_COMBS {
            out_l += self.comb_l[i].process(proxy_l);
            out_r += self.comb_r[i].process(proxy_r);
        }

        for i in 0..NUM_ALLPASSES {
            out_l = self.allpass_l[i].process(out_l);
            out_r = self.allpass_r[i].process(out_r);
        }

        let res_l = fma_f32(in_l, self.dry, fma_f32(out_l, self.wet1, out_r * self.wet2));
        let res_r = fma_f32(in_r, self.dry, fma_f32(out_r, self.wet1, out_l * self.wet2));
        (res_l, res_r)
    }

    /// Processes a single stereo sample through `processreplace` and `dsp_clip(..., 1.0)`.
    #[inline(always)]
    pub fn process_sample(&mut self, in_l: f32, in_r: f32) -> (f32, f32) {
        let (l, r) = self.process_sample_unclipped(in_l, in_r);
        (clip_sample(l, 1.0), clip_sample(r, 1.0))
    }

    /// Port of `mareverbe::processreplace`.
    pub fn process_replace(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
        skip: usize,
    ) {
        let step = skip.max(1);
        let n = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        let mut idx = 0;
        while idx < n {
            let (l, r) = self.process_sample_unclipped(in_l[idx], in_r[idx]);
            out_l[idx] = l;
            out_r[idx] = r;
            idx += step;
        }
    }

    /// Port of `mareverbe::processmix`.
    pub fn process_mix(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
        skip: usize,
    ) {
        let step = skip.max(1);
        let n = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        let mut idx = 0;
        while idx < n {
            let (l, r) = self.process_sample_unclipped(in_l[idx], in_r[idx]);
            out_l[idx] += l;
            out_r[idx] += r;
            idx += step;
        }
    }

    /// Port of `mareverbe::process_stereo`: `processreplace` with `skip = 1`, then `dsp_clip(..., 1.0)`.
    pub fn process_stereo(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let n = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        self.process_replace(&in_l[..n], &in_r[..n], &mut out_l[..n], &mut out_r[..n], 1);
        dsp_clip(&mut out_l[..n], 1.0);
        dsp_clip(&mut out_r[..n], 1.0);
    }

    /// In-place version of [`Mareverbe::process_stereo`].
    pub fn process_stereo_in_place(&mut self, buf_l: &mut [f32], buf_r: &mut [f32]) {
        let n = buf_l.len().min(buf_r.len());
        for i in 0..n {
            let (l, r) = self.process_sample_unclipped(buf_l[i], buf_r[i]);
            buf_l[i] = l;
            buf_r[i] = r;
        }
        dsp_clip(&mut buf_l[..n], 1.0);
        dsp_clip(&mut buf_r[..n], 1.0);
    }

    fn update(&mut self) {
        self.wet1 = self.wet * (self.width / 2.0 + 0.5);
        self.wet2 = self.wet * ((1.0 - self.width) / 2.0);

        if self.mode >= FREEZE_MODE {
            self.roomsize1 = 1.0;
            self.damp1 = 0.0;
            self.gain = MUTED;
        } else {
            self.roomsize1 = self.roomsize;
            self.damp1 = self.damp;
            self.gain = FIXED_GAIN;
        }

        for i in 0..NUM_COMBS {
            self.comb_l[i].set_feedback(self.roomsize1);
            self.comb_r[i].set_feedback(self.roomsize1);
        }

        for i in 0..NUM_COMBS {
            self.comb_l[i].set_damp(self.damp1);
            self.comb_r[i].set_damp(self.damp1);
        }
    }

    pub fn set_room_size(&mut self, value: f32) {
        let r = fma_f32(value, SCALE_ROOM, OFFSET_ROOM);
        if self.roomsize.to_bits() != r.to_bits() {
            self.roomsize = r;
            self.update();
        }
    }

    pub fn room_size(&self) -> f32 {
        (self.roomsize - OFFSET_ROOM) / SCALE_ROOM
    }

    pub fn set_damp(&mut self, value: f32) {
        let d = value * SCALE_DAMP;
        if self.damp.to_bits() != d.to_bits() {
            self.damp = d;
            self.update();
        }
    }

    pub fn damp(&self) -> f32 {
        self.damp / SCALE_DAMP
    }

    pub fn set_wet(&mut self, value: f32) {
        let w = value * SCALE_WET;
        if self.wet.to_bits() != w.to_bits() {
            self.wet = w;
            self.update();
        }
    }

    pub fn wet(&self) -> f32 {
        self.wet / SCALE_WET
    }

    pub fn set_dry(&mut self, value: f32) {
        self.dry = value * SCALE_DRY;
    }

    pub fn dry(&self) -> f32 {
        self.dry / SCALE_DRY
    }

    pub fn set_width(&mut self, value: f32) {
        if self.width.to_bits() != value.to_bits() {
            self.width = value;
            self.update();
        }
    }

    pub fn width(&self) -> f32 {
        self.width
    }

    pub fn set_mode(&mut self, value: f32) {
        if self.mode.to_bits() != value.to_bits() {
            self.mode = value;
            self.update();
        }
    }

    pub fn mode(&self) -> f32 {
        if self.mode >= FREEZE_MODE {
            1.0
        } else {
            0.0
        }
    }
}
