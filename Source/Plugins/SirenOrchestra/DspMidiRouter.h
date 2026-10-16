//
// Extra MIDI-in mapping for mixer DSP parameters (reverb, per-siren pan/gain,
// master volume). PR #16 / CustomMix CC numbers and channels.
//
// This is not MidiIn (the siren firmware CC table: volume, vibrato, notes).
// Messages handled here only move APVTS parameters; ParameterBridges already
// drive the DSP. Nothing is scheduled onto the siren MIDI output or the park.
//

#ifndef COMPOSESIREN_DSPMIDIROUTER_H
#define COMPOSESIREN_DSPMIDIROUTER_H

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

#include <apvtsUtilities.h>
#include <lib/definitions/parameterDefinitions.h>
#include <lib/definitions/sirenProperties.h>

class DspMidiRouter
{
    static constexpr int kSirenChannels = 7;
    static constexpr int kResetAllChannel = 16;

    std::array<juce::RangedAudioParameter*, kSirenChannels + 1> pan {};
    std::array<juce::RangedAudioParameter*, kSirenChannels + 1> gain {};
    juce::RangedAudioParameter* master = nullptr;
    juce::RangedAudioParameter* reverbEnable = nullptr;
    juce::RangedAudioParameter* reverbRoomSize = nullptr;
    juce::RangedAudioParameter* reverbDryWet = nullptr;
    juce::RangedAudioParameter* reverbDamping = nullptr;
    juce::RangedAudioParameter* reverbLowCut = nullptr;
    juce::RangedAudioParameter* reverbHighCut = nullptr;
    juce::RangedAudioParameter* reverbWidth = nullptr;

    static juce::RangedAudioParameter* param(juce::AudioProcessorValueTreeState& vts,
                                             const std::string& group,
                                             ParameterId id)
    {
        return vts.getParameter(ParameterIdGet::toJuceParameterId(group, id));
    }

    static void setNormalized(juce::RangedAudioParameter* p, int ccValue)
    {
        if (p == nullptr)
            return;
        p->setValueNotifyingHost(static_cast<float>(juce::jlimit(0, 127, ccValue)) / 127.0f);
    }

    static void setEnable(juce::RangedAudioParameter* p, int ccValue)
    {
        // PR #16: CC 64 on = value >= 64
        if (p == nullptr)
            return;
        p->setValueNotifyingHost(ccValue >= 64 ? 1.0f : 0.0f);
    }

public:
    explicit DspMidiRouter(juce::AudioProcessorValueTreeState& vts)
    {
        for (const auto sid : allSirenIds) {
            const int ch = sirenPropertiesById.at(sid)->oneBasedMidiChannel.oneBased;
            if (ch < 1 || ch > kSirenChannels)
                continue;
            const auto& group = sirenStrIdById.at(sid);
            pan[static_cast<size_t>(ch)] = param(vts, group, ParameterId::TrackPanning);
            gain[static_cast<size_t>(ch)] = param(vts, group, ParameterId::TrackOutputGain);
        }
        master = param(vts, "M", ParameterId::MasterVolume);
        reverbEnable = param(vts, "R", ParameterId::ReverbEnable);
        reverbRoomSize = param(vts, "R", ParameterId::ReverbRoomSize);
        reverbDryWet = param(vts, "R", ParameterId::ReverbDryWet);
        reverbDamping = param(vts, "R", ParameterId::ReverbDamping);
        reverbLowCut = param(vts, "R", ParameterId::ReverbLowCut);
        reverbHighCut = param(vts, "R", ParameterId::ReverbHighCut);
        reverbWidth = param(vts, "R", ParameterId::ReverbWidth);
    }

    // true when the message is a mixer DSP CC this router owns.
    // Notes, siren CCs (volume, vibrato, …) and CC 121 stay with MidiIn.
    bool handle(const juce::MidiMessage& msg) const
    {
        if (!msg.isController())
            return false;
        const int ch = msg.getChannel();
        const int cc = msg.getControllerNumber();
        const int value = msg.getControllerValue();

        if (ch >= 1 && ch <= kSirenChannels) {
            if (cc == 10) {
                setNormalized(pan[static_cast<size_t>(ch)], value);
                return true;
            }
            if (cc == 70) {
                setNormalized(gain[static_cast<size_t>(ch)], value);
                return true;
            }
            return false;
        }

        if (ch != kResetAllChannel)
            return false;

        switch (cc) {
            case 7:
                setNormalized(master, value);
                return true;
            case 64:
                setEnable(reverbEnable, value);
                return true;
            case 65:
                setNormalized(reverbRoomSize, value);
                return true;
            case 66:
                setNormalized(reverbDryWet, value);
                return true;
            case 67:
                setNormalized(reverbDamping, value);
                return true;
            case 68:
                setNormalized(reverbLowCut, value);
                return true;
            case 69:
                setNormalized(reverbHighCut, value);
                return true;
            case 70:
                setNormalized(reverbWidth, value);
                return true;
            default:
                return false;
        }
    }
};

#endif // COMPOSESIREN_DSPMIDIROUTER_H
