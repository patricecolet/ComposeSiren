//
// Extra MIDI-in port for DSP Control (DspControlMidiRouter).
//
// A virtual CoreMIDI/ALSA destination named "SirenOrchestra: DSP Control"
// (macOS/Linux); further plugin instances number the SirenOrchestra prefix
// ("SirenOrchestra 2: DSP Control", …). Optional hardware MIDI device from
// Settings. Incoming messages never enter MidiIn or OrchestraMidiRouter; the
// processor drains them into DspControlMidiRouter only.
//

#ifndef COMPOSESIREN_DSPCONTROLMIDIINPUT_H
#define COMPOSESIREN_DSPCONTROLMIDIINPUT_H

#include "VirtualMidiInput.h"

class DspControlMidiInput : public VirtualMidiInput
{
public:
    static constexpr const char* kRole = "DSP Control";

    /// instanceIndex 0: "SirenOrchestra: DSP Control"
    /// instanceIndex 1: "SirenOrchestra 2: DSP Control"
    static juce::String virtualDestinationName(int instanceIndex)
    {
        return VirtualMidiInput::virtualDestinationName(kRole, instanceIndex);
    }

    DspControlMidiInput() : VirtualMidiInput(kRole) {}
};

#endif // COMPOSESIREN_DSPCONTROLMIDIINPUT_H
