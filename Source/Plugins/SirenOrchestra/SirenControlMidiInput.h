//
// Extra MIDI-in port for Siren Control (MidiIn / OrchestraMidiRouter).
//
// A virtual CoreMIDI/ALSA destination named "SirenOrchestra: Siren Control"
// (macOS/Linux); further plugin instances number the SirenOrchestra prefix
// ("SirenOrchestra 2: Siren Control", …). Optional hardware MIDI device from
// Settings. Incoming messages join host MIDI: notes, volume, vibrato, and the
// other firmware CCs go through OrchestraMidiRouter, the park, and MIDI out.
// DSP Control CCs on this port stay ignored (same as host MIDI).
//

#ifndef COMPOSESIREN_SIRENCONTROLMIDIINPUT_H
#define COMPOSESIREN_SIRENCONTROLMIDIINPUT_H

#include "VirtualMidiInput.h"

class SirenControlMidiInput : public VirtualMidiInput
{
public:
    static constexpr const char* kRole = "Siren Control";

    /// instanceIndex 0: "SirenOrchestra: Siren Control"
    /// instanceIndex 1: "SirenOrchestra 2: Siren Control"
    static juce::String virtualDestinationName(int instanceIndex)
    {
        return VirtualMidiInput::virtualDestinationName(kRole, instanceIndex);
    }

    SirenControlMidiInput() : VirtualMidiInput(kRole) {}
};

#endif // COMPOSESIREN_SIRENCONTROLMIDIINPUT_H
