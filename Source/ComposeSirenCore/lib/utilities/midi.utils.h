//
// Created by joseph larralde on 02/03/2026.
//

#ifndef COMPOSESIREN_MIDI_UTILS_H
#define COMPOSESIREN_MIDI_UTILS_H

#include "math.utils.h"

// Missing juce MidiMessage::pitchwheelPosToPitchBend method
// (used to update slider / UI state from MIDI input)

static float pitchwheelPosToPitchbend(const int pitchwheelPos, float pitchbendRange)
{
    // see original MidiMessage::pitchbendToPitchwheelPos inverse conversion method
    auto pos = static_cast<float>(pitchwheelPos);
    return (
      pitchwheelPos > 8192
        ? linearMap (pos, 8192.0f, 16383.0f, 0.0f, pitchbendRange)
        : linearMap (pos, 0.0f, 8192.0f, -pitchbendRange, 0.0f)
    );
    // maybe this would be enough ?
    // return pitchbenRange * (pitchwheelPos - 8192) / 8192;
}

#endif //COMPOSESIREN_MIDI_UTILS_H
