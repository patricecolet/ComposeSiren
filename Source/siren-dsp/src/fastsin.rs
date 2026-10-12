//! A sine for the trio's fast path (`trio.fast`): range reduction and an odd
//! polynomial in plain f64 arithmetic, with no branch and no libm call, so the
//! compiler vectorizes the loops over partials (SSE2/AVX on x86_64, NEON on
//! aarch64; the same code runs scalar elsewhere). No target-specific
//! intrinsics, no allocation.
//!
//! Error: the reduction to [−π/2, π/2] is exact up to `k · ε(2π)` (about
//! 3e-13 for the trio's arguments, below 2000 rad), the degree-13 Taylor
//! polynomial adds at most 7e-10. `tests/fast.rs` bounds both.

use std::f64::consts::{FRAC_1_PI, PI, TAU};

/// 1.5 · 2^52: `(x + M) − M` rounds `x` to the nearest integer (ties to even)
/// for |x| < 2^51, in two additions that vectorize everywhere (SSE2 has no
/// rounding instruction)
const MAGIC: f64 = 6_755_399_441_055_744.0;

#[inline(always)]
pub fn round(x: f64) -> f64 {
    (x + MAGIC) - MAGIC
}

/// sin(x) for |x| < 2^50
#[inline(always)]
pub fn sin(x: f64) -> f64 {
    let r = x - round(x * (0.5 * FRAC_1_PI)) * TAU; // [−π, π]
    // sin(π − r) = sin(r): fold onto [−π/2, π/2] without a branch
    let r = r.min(PI - r).max(-PI - r);
    let r2 = r * r;
    const C3: f64 = -1.0 / 6.0;
    const C5: f64 = 1.0 / 120.0;
    const C7: f64 = -1.0 / 5040.0;
    const C9: f64 = 1.0 / 362_880.0;
    const C11: f64 = -1.0 / 39_916_800.0;
    const C13: f64 = 1.0 / 6_227_020_800.0;
    r * (1.0 + r2 * (C3 + r2 * (C5 + r2 * (C7 + r2 * (C9 + r2 * (C11 + r2 * C13))))))
}

/// Σ sin(phase[i]) · amp[i], over four independent sums (vector lanes)
#[inline]
pub fn sin_dot(phase: &[f64], amp: &[f64]) -> f64 {
    let n = phase.len().min(amp.len());
    let (phase, amp) = (&phase[..n], &amp[..n]);
    let mut acc = [0.0f64; 4];
    let mut pc = phase.chunks_exact(4);
    let mut ac = amp.chunks_exact(4);
    for (p, a) in (&mut pc).zip(&mut ac) {
        for j in 0..4 {
            acc[j] += sin(p[j]) * a[j];
        }
    }
    let mut s = (acc[0] + acc[1]) + (acc[2] + acc[3]);
    for (p, a) in pc.remainder().iter().zip(ac.remainder()) {
        s += sin(*p) * a;
    }
    s
}
