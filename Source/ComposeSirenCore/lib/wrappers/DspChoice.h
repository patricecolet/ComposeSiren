#pragma once

// Which synthesis the siren voices play, and on which data set (COMPOSESIREN_RUST_DSP):
// the dsp.variant and dsp.dataset settings, app-wide. Settings publishes them here; each
// voice reads them when its siren is loaded, so a change applies to the sirens loaded
// after it. Without COMPOSESIREN_SETTINGS the environment picks them
// (COMPOSESIREN_DSP_VARIANT, COMPOSESIREN_DSP_DATASET), else "cpp" and "fixed".

#include <string>

namespace cs {

struct DspChoice {
    std::string variant { "cpp" };  // an id of the DSP variants ("cpp": the C++ Sirene)
    std::string dataset { "fixed" }; // an id of the data sets
    bool isCpp() const { return variant == "cpp"; }
};

DspChoice currentDspChoice();       // any thread but the audio one (takes a lock)
void setDspChoice(DspChoice choice);

} // namespace cs
