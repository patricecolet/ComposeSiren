#pragma once

// Which synthesis the siren voices play, and on which data set (COMPOSESIREN_RUST_DSP):
// the dsp.variant, dsp.dataset and dsp.skip_silent_frames settings, app-wide. Settings publishes them here; each
// voice reads them when its siren is loaded, and the plugins reload every voice on a change
// (SirenVoice::reloadDsp: built off the audio thread, swapped at a free note-on).
// Until the settings load (and in tests without them) the environment picks them
// (COMPOSESIREN_DSP_VARIANT, COMPOSESIREN_DSP_DATASET, COMPOSESIREN_DSP_SKIP_SILENT=1),
// else "cpp", "fixed" and no skipping.

#include <string>

namespace cs {

struct DspChoice {
    std::string variant { "cpp" };  // an id of the DSP variants ("cpp": the C++ Sirene)
    std::string dataset { "fixed" }; // an id of the data sets
    bool skipSilent { false };       // the Rust synthesis skips practically silent frames (C++: no effect)
    bool isCpp() const { return variant == "cpp"; }
};

DspChoice currentDspChoice();       // any thread but the audio one (takes a lock)
void setDspChoice(DspChoice choice);

} // namespace cs
