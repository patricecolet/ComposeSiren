//! `RenderSpecC` (ffi.rs) can be filled by callers that write fixed offsets
//! (a JavaScript DataView over a WebAssembly build, for instance), so the
//! layout the compiler produces is pinned here: if a field moves, this fails
//! before a render silently comes out garbled.

use std::mem::{align_of, offset_of, size_of};

use siren_dsp::ffi::RenderSpecC;

#[test]
fn render_spec_c_layout_is_pinned() {
    assert_eq!(offset_of!(RenderSpecC, note), 0);
    assert_eq!(offset_of!(RenderSpecC, cents), 4);
    assert_eq!(offset_of!(RenderSpecC, velocity), 8);
    assert_eq!(offset_of!(RenderSpecC, seconds), 16);
    assert_eq!(offset_of!(RenderSpecC, release_seconds), 24);
    assert_eq!(offset_of!(RenderSpecC, sample_rate), 32);
    assert_eq!(offset_of!(RenderSpecC, partials), 40);
    assert_eq!(offset_of!(RenderSpecC, crossfade), 44);
    assert_eq!(offset_of!(RenderSpecC, bit16), 45);
    assert_eq!(offset_of!(RenderSpecC, glide), 46);
    assert_eq!(offset_of!(RenderSpecC, frames), 48);
    assert_eq!(offset_of!(RenderSpecC, start), 52);
    assert_eq!(offset_of!(RenderSpecC, end), 56);
    assert_eq!(offset_of!(RenderSpecC, freeze), 60);
    assert_eq!(offset_of!(RenderSpecC, speed), 64);
    assert_eq!(offset_of!(RenderSpecC, interpolate), 68);
    assert_eq!(offset_of!(RenderSpecC, skip_silent), 69);
    assert_eq!(offset_of!(RenderSpecC, smoothing), 72);
    assert_eq!(size_of::<RenderSpecC>(), 80);
    assert_eq!(align_of::<RenderSpecC>(), 8);
}
