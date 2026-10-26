//! `reverb-dsp`: ComposeSiren's stereo reverb (`Source/ComposeSirenCore/Reverb.cpp` and
//! `lib/dsp/mareverbe.cpp`), ported to Rust sample for sample.
//!
//! - [`mareverbe`]: the Freeverb stereo core ([`Mareverbe`]), its 8 parallel feedback [`Comb`]
//!   filters and 4 series [`Allpass`] filters per channel, plus [`mareverbe::dsp_clip`].
//! - [`biquad`]: the 2nd-order Butterworth low-pass and high-pass filters ([`IirFilter`],
//!   [`IirCoefficients`]) matching `juce::IIRFilter` / `juce::IIRCoefficients`.
//! - [`rampe`]: the linear parameter ramp ([`Rampe`]) matching `rampe<float>`.
//! - [`reverb`]: the complete [`Reverb`] processor used by SirenOrchestra (bit-exact with C++
//!   in [`ReverbMode::Plugin`], or with independent stereo high-pass and sample-rate-scaled
//!   ramps in [`ReverbMode::StereoTime`]).
//! - [`ffi`]: C ABI (`csr_reverb_*`, `csr_mareverbe_*`) declared in `include/reverb_dsp.h`.

pub mod biquad;
pub mod ffi;
pub mod mareverbe;
pub mod rampe;
pub mod reverb;

pub use biquad::{IirCoefficients, IirFilter};
pub use mareverbe::{Allpass, Comb, Mareverbe};
pub use rampe::Rampe;
pub use reverb::{Reverb, ReverbMode};
