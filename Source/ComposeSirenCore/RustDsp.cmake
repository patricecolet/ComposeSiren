# siren-dsp and reverb-dsp: the Rust synthesis (Source/siren-dsp) and stereo reverb
# (Source/reverb-dsp), linked into ComposeSirenCore when COMPOSESIREN_RUST_DSP is ON.
# The voice (lib/wrappers/SirenVoice) and Reverb (Reverb.cpp) play them in place of the
# C++ Sirene and Reverb when the dsp.variant setting names a Rust variant; the C headers
# are include/siren_dsp_plugin.h and include/reverb_dsp.h.

include("${CMAKE_SOURCE_DIR}/cmake/RustStaticLib.cmake")

composesiren_add_rust_staticlib(
  TARGET siren_dsp
  CRATE_DIR "${CMAKE_SOURCE_DIR}/Source/siren-dsp"
)

composesiren_add_rust_staticlib(
  TARGET reverb_dsp
  CRATE_DIR "${CMAKE_SOURCE_DIR}/Source/reverb-dsp"
)
