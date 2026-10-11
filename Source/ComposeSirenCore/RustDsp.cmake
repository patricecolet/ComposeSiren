# siren-dsp: the Rust synthesis (Source/siren-dsp), linked into ComposeSirenCore when
# COMPOSESIREN_RUST_DSP is ON. The voice (lib/wrappers/SirenVoice) plays it in place of the
# C++ Sirene when the dsp.variant setting names a Rust variant; the C header is the crate's
# include/siren_dsp_plugin.h.

include("${CMAKE_SOURCE_DIR}/cmake/RustStaticLib.cmake")

composesiren_add_rust_staticlib(
  TARGET siren_dsp
  CRATE_DIR "${CMAKE_SOURCE_DIR}/Source/siren-dsp"
)
