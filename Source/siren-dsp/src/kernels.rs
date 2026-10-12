//! Three ways to compute the same held note (the lab's loop of frames, at
//! the note's own pitch, 16-bit mode, no crossfade, smoothing 0.001), to
//! measure where the time goes:
//!
//! - [`ScalarSin`]: `calculwave`'s arithmetic as is, one libm `sin` per
//!   partial per sample, over any [`FrameSource`]. Sample-exact with the
//!   port (and so with the plugin) in that configuration, at
//!   [`crate::sirene::Timing::Plugin`]: the kernels count a frame's `fdur`
//!   and the smoothing in samples at any rate (they measure time per sample).
//! - [`PhasorPartials`]: no `sin` in the loop. Each partial is a unit
//!   complex number turned by `w = e^{iω}` every sample; `w` is computed
//!   once per frame (polynomial sin/cos, vectorized over partials). The
//!   sample loop runs over partials in SIMD lanes, then sums the lanes.
//! - [`PhasorTime`]: the same phasors, but the SIMD lanes are 16 successive
//!   samples of one partial: `z·w^1..z·w^16` with `w^k` tabulated per frame,
//!   and the amplitude smoothing in closed form (`ampz` approaches `amp`
//!   geometrically while a frame lasts: `amp + (ampz − amp)·b^k`).
//!
//! The phasors are f32. Left alone they drift from the original's f64 phase
//! by rounding, linearly with time (−46 dB after 9 s on the piccolo's top
//! notes). So each frame re-anchors them: an f64 phase per partial advances
//! by a whole frame at once (one multiply-add per partial per frame, as the
//! original adds per sample), and the phasor restarts from its cos/sin. The
//! error then stays that of one frame: `tests/kernels.rs` bounds it.

use std::ops::Deref;

use crate::layout::{Bank, FrameSource, LANES};

/// What every kernel renders: one note, frames `[start, end)` in a loop.
#[derive(Clone, Copy, Debug)]
pub struct Held {
    pub note: usize,
    pub start: usize,
    pub end: usize,
    pub partials: usize,
    pub sample_rate: f64,
}

impl Held {
    /// `pitchSchift` at the note's exact pitch: `2π/sr`, as `setMidicent`
    /// rounds it
    fn pitch_schift(&self) -> f32 {
        let m = self.note as i32 * 100;
        let wanted = 440.0 * 2f64.powf(((m as f64 / 100.0) - 69.0) / 12.0);
        let at = 440.0 * 2f64.powf((self.note as f64 - 69.0) / 12.0);
        ((wanted / at) * (2.0 * std::f64::consts::PI / self.sample_rate)) as f32
    }
}

const SMOOTH_A: f64 = 0.001;
const SMOOTH_B: f64 = 0.999;

/// frame counter of `calculwave` (lab loop)
#[derive(Clone, Copy)]
struct Counter {
    frame: usize,
    k: usize,
    fdur: usize,
    start: usize,
    end: usize,
}

impl Counter {
    fn new(h: &Held, fdur: usize) -> Counter {
        Counter { frame: h.start, k: 0, fdur, start: h.start, end: h.end.max(h.start + 1) }
    }
    /// samples left in the current frame, before the counter moves
    #[inline]
    fn left(&self) -> usize {
        if self.fdur == 0 { usize::MAX } else { self.fdur - self.k }
    }
    #[inline]
    fn step(&mut self, n: usize) -> bool {
        self.k += n;
        if self.fdur > 0 && self.k >= self.fdur {
            self.k = 0;
            self.frame = if self.frame + 1 >= self.end { self.start } else { self.frame + 1 };
            return true;
        }
        false
    }
}

pub trait Kernel {
    fn render(&mut self, out: &mut [f32]);
}

// --- scalar, libm sin ----------------------------------------------------------

pub struct ScalarSin<S: FrameSource> {
    src: S,
    h: Held,
    ps: f32,
    c: Counter,
    phase: Vec<f64>,
    ampz: Vec<f32>,
}

impl<S: FrameSource> ScalarSin<S> {
    pub fn new(src: S, h: Held) -> Self {
        let c = Counter::new(&h, src.fdur(h.note));
        ScalarSin { src, h, ps: h.pitch_schift(), c, phase: vec![0.0; h.partials], ampz: vec![0.0; h.partials] }
    }
}

impl<S: FrameSource> Kernel for ScalarSin<S> {
    fn render(&mut self, out: &mut [f32]) {
        let ps = self.ps;
        for o in out.iter_mut() {
            let (freq, amp) = self.src.frame(self.h.note, self.c.frame);
            let q = self.h.partials.min(freq.len());
            let mut wave = 0.0f32;
            for i in 0..q {
                self.phase[i] += (freq[i] * ps) as f64;
                self.ampz[i] = (SMOOTH_A * amp[i] as f64 + SMOOTH_B * self.ampz[i] as f64) as f32;
                wave = (wave as f64 + self.phase[i].sin() * self.ampz[i] as f64) as f32;
            }
            *o = wave;
            self.c.step(1);
        }
    }
}

// --- vectorizable sin/cos for 0 <= x <= π ----------------------------------------

/// sin on [-π/2, π/2], Taylor to x^11 (error < 6e-10 there)
#[inline(always)]
fn sin_poly(x: f32) -> f32 {
    let x2 = x * x;
    let p = -1.0 / 39916800.0;
    let p = p * x2 + 1.0 / 362880.0;
    let p = p * x2 - 1.0 / 5040.0;
    let p = p * x2 + 1.0 / 120.0;
    let p = p * x2 - 1.0 / 6.0;
    x + x * x2 * p
}

/// (cos x, sin x) for x in [0, π], no branch
#[inline(always)]
fn cos_sin(x: f32) -> (f32, f32) {
    let half_pi = std::f32::consts::FRAC_PI_2;
    let s = sin_poly(x.min(std::f32::consts::PI - x));
    let c = sin_poly(half_pi - x);
    (c, s)
}

/// (cos φ, sin φ) for any φ, and φ brought back to [0, 2π)
#[inline(always)]
pub(crate) fn anchor(phi: f64) -> (f64, f32, f32) {
    use std::f64::consts::TAU;
    let p = phi - (phi * (1.0 / TAU)).floor() * TAU;
    let x = p as f32;
    let pi = std::f32::consts::PI;
    // on (π, 2π): cos(x) = cos(2π − x), sin(x) = −sin(2π − x)
    let upper = x > pi;
    let (c, s) = cos_sin(if upper { 2.0 * pi - x } else { x });
    (p, c, if upper { -s } else { s })
}

// --- phasors, SIMD over partials ---------------------------------------------------

pub struct PhasorPartials<B: Deref<Target = Bank>> {
    bank: B,
    h: Held,
    ps: f32,
    c: Counter,
    stride: usize,
    zr: Vec<f32>,
    zi: Vec<f32>,
    az: Vec<f32>,
    wr: Vec<f32>,
    wi: Vec<f32>,
    amp: Vec<f32>,
    /// f64 phase at the start of the frame, and its per-sample increment
    phi: Vec<f64>,
    inc: Vec<f64>,
}

impl<B: Deref<Target = Bank>> PhasorPartials<B> {
    pub fn new(bank: B, h: Held) -> Self {
        let stride = h.partials.min(bank.stride(h.note)).div_ceil(LANES) * LANES;
        let c = Counter::new(&h, bank.fdur(h.note));
        let mut k = PhasorPartials {
            bank,
            h,
            ps: h.pitch_schift(),
            c,
            stride,
            zr: vec![1.0; stride],
            zi: vec![0.0; stride],
            az: vec![0.0; stride],
            wr: vec![1.0; stride],
            wi: vec![0.0; stride],
            amp: vec![0.0; stride],
            phi: vec![0.0; stride],
            inc: vec![0.0; stride],
        };
        k.load_frame(0);
        k
    }

    /// new frame, `elapsed` samples after the last: `w` and `amp` per
    /// partial, and the phasors re-anchored on the f64 phase
    fn load_frame(&mut self, elapsed: usize) {
        let (freq, amp) = self.bank.frame(self.h.note, self.c.frame);
        let q = self.h.partials.min(freq.len());
        let ps = self.ps;
        for i in 0..self.stride {
            let (f, a) = if i < q { (freq[i], amp[i]) } else { (0.0, 0.0) };
            let (p, zr, zi) = anchor(self.phi[i] + self.inc[i] * elapsed as f64);
            self.phi[i] = p;
            self.zr[i] = zr;
            self.zi[i] = zi;
            let w = f * ps;
            self.inc[i] = w as f64;
            let (c, s) = cos_sin(w);
            self.wr[i] = c;
            self.wi[i] = s;
            self.amp[i] = a;
        }
    }
}

impl<B: Deref<Target = Bank>> Kernel for PhasorPartials<B> {
    fn render(&mut self, out: &mut [f32]) {
        let (a, b) = (SMOOTH_A as f32, SMOOTH_B as f32);
        let n = self.stride;
        let mut t = 0;
        while t < out.len() {
            let len = self.c.left().min(out.len() - t);
            let (zr, zi, az) = (&mut self.zr[..n], &mut self.zi[..n], &mut self.az[..n]);
            let (wr, wi, amp) = (&self.wr[..n], &self.wi[..n], &self.amp[..n]);
            for o in &mut out[t..t + len] {
                let mut acc = [0.0f32; LANES];
                for c in 0..n / LANES {
                    let r = c * LANES..(c + 1) * LANES;
                    let (zr, zi, az) = (&mut zr[r.clone()], &mut zi[r.clone()], &mut az[r.clone()]);
                    let (wr, wi, amp) = (&wr[r.clone()], &wi[r.clone()], &amp[r]);
                    for j in 0..LANES {
                        let re = zr[j] * wr[j] - zi[j] * wi[j];
                        let im = zr[j] * wi[j] + zi[j] * wr[j];
                        zr[j] = re;
                        zi[j] = im;
                        az[j] = a * amp[j] + b * az[j];
                        acc[j] += az[j] * im;
                    }
                }
                *o = acc.iter().sum();
            }
            t += len;
            let fdur = self.c.fdur;
            if self.c.step(len) {
                self.load_frame(fdur);
            }
        }
    }
}

// --- phasors, SIMD over time ----------------------------------------------------------

pub struct PhasorTime<B: Deref<Target = Bank>> {
    bank: B,
    h: Held,
    ps: f32,
    c: Counter,
    q: usize,
    zr: Vec<f32>,
    zi: Vec<f32>,
    az: Vec<f32>,
    amp: Vec<f32>,
    /// per partial, `w^1..w^LANES`
    pr: Vec<[f32; LANES]>,
    pi: Vec<[f32; LANES]>,
    /// `b^1..b^LANES`
    bk: [f32; LANES],
    phi: Vec<f64>,
    inc: Vec<f64>,
}

impl<B: Deref<Target = Bank>> PhasorTime<B> {
    pub fn new(bank: B, h: Held) -> Self {
        let q = h.partials.min(bank.stride(h.note));
        let mut bk = [0.0f32; LANES];
        let mut p = 1.0f64;
        for x in &mut bk {
            p *= SMOOTH_B;
            *x = p as f32;
        }
        let c = Counter::new(&h, bank.fdur(h.note));
        let mut k = PhasorTime {
            bank,
            h,
            ps: h.pitch_schift(),
            c,
            q,
            zr: vec![1.0; q],
            zi: vec![0.0; q],
            az: vec![0.0; q],
            amp: vec![0.0; q],
            pr: vec![[0.0; LANES]; q],
            pi: vec![[0.0; LANES]; q],
            bk,
            phi: vec![0.0; q],
            inc: vec![0.0; q],
        };
        k.load_frame(0);
        k
    }

    fn load_frame(&mut self, elapsed: usize) {
        let (freq, amp) = self.bank.frame(self.h.note, self.c.frame);
        let ps = self.ps;
        for i in 0..self.q {
            let (f, a) = if i < freq.len() { (freq[i], amp[i]) } else { (0.0, 0.0) };
            self.amp[i] = a;
            let (p, zr, zi) = anchor(self.phi[i] + self.inc[i] * elapsed as f64);
            self.phi[i] = p;
            self.zr[i] = zr;
            self.zi[i] = zi;
            let w = f * ps;
            self.inc[i] = w as f64;
            let (c, s) = cos_sin(w);
            let (pr, pi) = (&mut self.pr[i], &mut self.pi[i]);
            pr[0] = c;
            pi[0] = s;
            for k in 1..LANES {
                pr[k] = pr[k - 1] * c - pi[k - 1] * s;
                pi[k] = pr[k - 1] * s + pi[k - 1] * c;
            }
        }
    }
}

impl<B: Deref<Target = Bank>> Kernel for PhasorTime<B> {
    fn render(&mut self, out: &mut [f32]) {
        out.fill(0.0);
        let mut t = 0;
        while t < out.len() {
            let len = self.c.left().min(out.len() - t);
            let seg = &mut out[t..t + len];
            for i in 0..self.q {
                let (pr, pi) = (&self.pr[i], &self.pi[i]);
                let amp = self.amp[i];
                let (mut zr, mut zi) = (self.zr[i], self.zi[i]);
                let mut d = self.az[i] - amp;
                let mut chunks = seg.chunks_exact_mut(LANES);
                for o in &mut chunks {
                    for k in 0..LANES {
                        let im = zr * pi[k] + zi * pr[k];
                        o[k] += (amp + d * self.bk[k]) * im;
                    }
                    let re = zr * pr[LANES - 1] - zi * pi[LANES - 1];
                    zi = zr * pi[LANES - 1] + zi * pr[LANES - 1];
                    zr = re;
                    d *= self.bk[LANES - 1];
                }
                let rest = chunks.into_remainder();
                if !rest.is_empty() {
                    let n = rest.len();
                    for (k, o) in rest.iter_mut().enumerate() {
                        let im = zr * pi[k] + zi * pr[k];
                        *o += (amp + d * self.bk[k]) * im;
                    }
                    let re = zr * pr[n - 1] - zi * pi[n - 1];
                    zi = zr * pi[n - 1] + zi * pr[n - 1];
                    zr = re;
                    d *= self.bk[n - 1];
                }
                self.zr[i] = zr;
                self.zi[i] = zi;
                self.az[i] = amp + d;
            }
            t += len;
            let fdur = self.c.fdur;
            if self.c.step(len) {
                self.load_frame(fdur);
            }
        }
    }
}
