//! ComposeSiren's click output: the click box's engine (`clic-core` from
//! firmwares-artila's `ClicRaspberry/clic-core`), driven by MIDI, rendered to a
//! stereo bus.
//!
//! The MIDI rule is the box's: channel 10, note on with note > 1 and
//! velocity > 1, an even note plays the strong click, an odd one the weak
//! click; a program change on channel 10 picks the click (0 = the first).
//!
//! [`Clic::midi`] and [`Clic::render`] neither allocate nor block: they run on
//! the audio thread. [`Clic::set_sample_rate`] reloads the clicks at the new
//! rate, from `prepareToPlay`.

use clic_core::{Accent, Bank, Player};

mod ffi;

/// Frames rendered per pass through the interleaved stack buffer.
const CHUNK: usize = 256;

/// The click engine of one plugin instance. The strong (clic1) and weak
/// (clic2) voices are separate players so each can be panned and shortened
/// by its own decay envelope.
pub struct Clic {
    fort: Player,
    faible: Player,
    env_fort: f32,
    env_faible: f32,
    curve: DecayCurve,
    rate: u32,
}

impl Clic {
    /// The built-in clicks, at `sample_rate`.
    #[must_use]
    pub fn new(sample_rate: f64) -> Clic {
        let rate = to_rate(sample_rate);
        let bank = Bank::embedded();
        Clic {
            fort: Player::new(bank, rate),
            faible: Player::new(bank, rate),
            env_fort: 0.0,
            env_faible: 0.0,
            curve: DecayCurve::Exponential,
            rate,
        }
    }

    /// Fade shape for `decay`. Default is Exponential (musical amplitude
    /// decay). Bank preset metadata can override per sample later.
    pub fn set_decay_curve(&mut self, curve: DecayCurve) {
        self.curve = curve;
    }

    /// Fade shape for `decay`.
    #[must_use]
    pub fn decay_curve(&self) -> DecayCurve {
        self.curve
    }

    /// Reloads the clicks at `sample_rate`, keeping the chosen click.
    pub fn set_sample_rate(&mut self, sample_rate: f64) {
        let rate = to_rate(sample_rate);
        if rate != self.rate {
            let current = self.fort.current();
            self.fort = Player::new(Bank::embedded(), rate);
            self.faible = Player::new(Bank::embedded(), rate);
            let _ = self.fort.set_clic(current);
            let _ = self.faible.set_clic(current);
            self.rate = rate;
        }
    }

    /// A MIDI message; anything but the click's is ignored.
    pub fn midi(&mut self, status: u8, data1: u8, data2: u8) {
        match (status, data1, data2) {
            // Note on, channel 10, note > 1 and velocity > 1: even = strong.
            (0x99, note, velocity) if note > 1 && velocity > 1 => {
                if note % 2 == 0 {
                    self.fort.trigger(Accent::Fort);
                    self.env_fort = 1.0;
                } else {
                    self.faible.trigger(Accent::Faible);
                    self.env_faible = 1.0;
                }
            }
            // Program change, channel 10: an unknown click is ignored.
            (0xc9, program, _) => {
                let _ = self.fort.set_clic(usize::from(program));
                let _ = self.faible.set_clic(usize::from(program));
            }
            _ => {}
        }
    }

    /// Renders the next `min(left.len(), right.len())` frames, both clicks
    /// centred (their own stereo image), full length.
    pub fn render(&mut self, left: &mut [f32], right: &mut [f32]) {
        self.render_panned(left, right, 0.0, 0.0, 1.0);
    }

    /// Renders with the two clicks placed in the stereo field.
    ///
    /// * `spread` (−1..=1): +1 puts clic1 (strong) left and clic2 (weak)
    ///   right; −1 the opposite; 0 both centre.
    /// * `bias` (−1..=1): moves both left (−1) or right (+1).
    /// * `decay` (0..=1): 1 plays the full click sample; 0 fades it out very
    ///   fast (a couple of milliseconds) so it is only just audible.
    ///
    /// Each click is mixed to mono and placed with an equal-power pan, so
    /// spread and bias describe the pair instead of two independent pans.
    pub fn render_panned(&mut self, left: &mut [f32], right: &mut [f32], spread: f32, bias: f32, decay: f32) {
        let pan1 = (bias - spread).clamp(-1.0, 1.0);
        let pan2 = (bias + spread).clamp(-1.0, 1.0);
        let (l1, r1) = pan_gains(pan1);
        let (l2, r2) = pan_gains(pan2);
        let fade = decay_coeff(decay.clamp(0.0, 1.0), self.rate);

        let mut block = [0.0f32; 2 * CHUNK];
        for (l, r) in left.chunks_mut(CHUNK).zip(right.chunks_mut(CHUNK)) {
            let n = l.len().min(r.len());
            self.fort.render_f32(&mut block[..2 * n]);
            for (i, frame) in block[..2 * n].chunks_exact(2).enumerate() {
                let mid = 0.5 * (frame[0] + frame[1]);
                let g = shaped(self.env_fort, self.curve);
                l[i] = mid * l1 * g;
                r[i] = mid * r1 * g;
                self.env_fort *= fade;
            }
            self.faible.render_f32(&mut block[..2 * n]);
            for (i, frame) in block[..2 * n].chunks_exact(2).enumerate() {
                let mid = 0.5 * (frame[0] + frame[1]);
                let g = shaped(self.env_faible, self.curve);
                l[i] = (l[i] + mid * l2 * g).clamp(-1.0, 1.0);
                r[i] = (r[i] + mid * r2 * g).clamp(-1.0, 1.0);
                self.env_faible *= fade;
            }
        }
    }

    /// The chosen click.
    #[must_use]
    pub fn current(&self) -> usize {
        self.fort.current()
    }

    /// How many clicks there are.
    #[must_use]
    pub fn count(&self) -> usize {
        Bank::embedded().len()
    }
}

/// Equal-power pan gains for `pan` in −1 (left) ..= 1 (right).
fn pan_gains(pan: f32) -> (f32, f32) {
    let angle = (pan + 1.0) * std::f32::consts::FRAC_PI_4;
    (angle.cos(), angle.sin())
}

/// How the click fades after its attack. Exponential is the default (musical
/// amplitude decay). Per-sample in the bank's preset metadata later.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DecayCurve {
    /// Straight ramp to silence over the fade window.
    Linear,
    /// Natural amplitude decay (e^-t) — default for clicks.
    Exponential,
    /// Fast drop then a long tail (log-shaped).
    Logarithmic,
    /// Smooth S-curve fade (sigmoid).
    Sigmoid,
}

/// Shapes a linear envelope sample for `curve` (0..=1).
fn shaped(g: f32, curve: DecayCurve) -> f32 {
    match curve {
        DecayCurve::Linear | DecayCurve::Exponential => g,
        DecayCurve::Logarithmic => g.sqrt(),
        DecayCurve::Sigmoid => g * g * (3.0 - 2.0 * g),
    }
}

/// Per-sample multiplier that shortens the click. `decay` 1 = 1.0 (full
/// sample). `decay` 0 ≈ 2 ms to −60 dB — a tick that is only just audible.
fn decay_coeff(decay: f32, rate: u32) -> f32 {
    if decay >= 0.999 {
        return 1.0;
    }
    // time constant from 2 ms (decay 0) up to ~0.5 s (decay → 1)
    let seconds = 0.002 + f32::from(decay) * 0.5;
    (-1.0 / (seconds * rate as f32)).exp()
}

#[allow(
    clippy::cast_possible_truncation,
    clippy::cast_sign_loss,
    reason = "bounded to 1..=u32::MAX just before"
)]
fn to_rate(sample_rate: f64) -> u32 {
    if sample_rate.is_finite() && (1.0..=f64::from(u32::MAX)).contains(&sample_rate) {
        sample_rate.round() as u32
    } else {
        48_000
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn render(c: &mut Clic, n: usize) -> Vec<f32> {
        let (mut l, mut r) = (vec![0f32; n], vec![0f32; n]);
        c.render(&mut l, &mut r);
        l
    }

    #[test]
    fn la_regle_du_boitier() {
        let mut c = Clic::new(48_000.0);
        assert!(render(&mut c, 512).iter().all(|&x| x == 0.0));
        c.midi(0x98, 36, 100); // canal 9 : rien
        assert!(render(&mut c, 512).iter().all(|&x| x == 0.0));
        c.midi(0x99, 36, 100); // canal 10, note paire : clic fort
        assert!(render(&mut c, 512).iter().any(|&x| x != 0.0));
        c.midi(0xc9, 1, 0);
        assert_eq!(c.current(), 1);
        c.midi(0xc9, 9, 0); // clic absent : ignore
        assert_eq!(c.current(), 1);
    }

    #[test]
    fn spread_places_the_two_clicks() {
        let mut c = Clic::new(48_000.0);
        // clic fort only, hard left
        c.midi(0x99, 36, 100);
        let (mut l, mut r) = (vec![0f32; 512], vec![0f32; 512]);
        c.render_panned(&mut l, &mut r, 1.0, 0.0, 1.0);
        let (el, er) = (l.iter().map(|x| x.abs()).sum::<f32>(), r.iter().map(|x| x.abs()).sum::<f32>());
        assert!(el > 10.0 * er, "fort should be on the left: L={el} R={er}");

        let mut c = Clic::new(48_000.0);
        // clic faible only, hard right
        c.midi(0x99, 37, 100);
        c.render_panned(&mut l, &mut r, 1.0, 0.0, 1.0);
        let (el, er) = (l.iter().map(|x| x.abs()).sum::<f32>(), r.iter().map(|x| x.abs()).sum::<f32>());
        assert!(er > 10.0 * el, "faible should be on the right: L={el} R={er}");
    }

    #[test]
    fn decay_zero_shortens_the_click() {
        let loud = |decay: f32| {
            let mut c = Clic::new(48_000.0);
            c.midi(0x99, 36, 100);
            let (mut l, mut r) = (vec![0f32; 4800], vec![0f32; 4800]);
            c.render_panned(&mut l, &mut r, 0.0, 0.0, decay);
            l.iter().map(|x| x.abs()).sum::<f32>()
        };
        let full = loud(1.0);
        let tick = loud(0.0);
        assert!(full > 5.0 * tick, "decay 0 should be much quieter than decay 1: full={full} tick={tick}");
    }

    #[test]
    fn frequence_de_l_hote() {
        let mut c = Clic::new(44_100.0);
        c.midi(0xc9, 1, 0);
        c.set_sample_rate(96_000.0);
        assert_eq!(c.current(), 1);
        c.midi(0x99, 36, 100);
        let out = render(&mut c, 8000);
        let last = out.iter().rposition(|&x| x != 0.0).unwrap();
        // 70 ms de clave a 96 kHz
        assert!((6700..=6730).contains(&last), "{last}");
    }
}
