//
// Extra MIDI-in mapping for DSP Control: ParameterClass TrackControl,
// ReverbControl, and MasterControl (pan/gain, reverb, master volume).
// CC numbers come from CCParam on those rows of parameterDefinitions.
// Channel 1–7 are the sirens; channel 16 is the orchestra mixer bus
// (ReverbControl and MasterControl).
//
// SirenControl stays with MidiIn (firmware CCs: volume, vibrato, notes).
// Messages handled here only move APVTS parameters; ParameterBridges
// already drive the DSP. Nothing is scheduled onto the siren MIDI output
// or the park.
//

#ifndef COMPOSESIREN_DSPCONTROLMIDIROUTER_H
#define COMPOSESIREN_DSPCONTROLMIDIROUTER_H

#include <array>
#include <string>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include <apvtsUtilities.h>
#include <lib/definitions/parameterDefinitions.h>
#include <lib/definitions/sirenProperties.h>

class DspControlMidiRouter
{
    static constexpr int kSirenChannels = 7;
    static constexpr int kMixerChannel = 16;
    static constexpr int kCcCount = 128;

    using CcParams = std::array<juce::RangedAudioParameter*, kCcCount>;
    std::array<CcParams, kSirenChannels + 1> track {};
    CcParams mixer {};
    std::array<bool, kCcCount> toggleCc {};

    static int midiChannelForSirenGroup(const std::string& groupId)
    {
        for (const auto sid : allSirenIds) {
            if (sirenStrIdById.at(sid) == groupId)
                return sirenPropertiesById.at(sid)->oneBasedMidiChannel.oneBased;
        }
        return 0;
    }

    static bool isDspControlClass(ParameterClass c)
    {
        return c == ParameterClass::TrackControl
            || c == ParameterClass::ReverbControl
            || c == ParameterClass::MasterControl;
    }

    static void apply(juce::RangedAudioParameter* p, int ccValue, bool toggle)
    {
        if (p == nullptr)
            return;
        if (toggle)
            p->setValueNotifyingHost(ccValue >= 64 ? 1.0f : 0.0f);
        else
            p->setValueNotifyingHost(static_cast<float>(juce::jlimit(0, 127, ccValue)) / 127.0f);
    }

public:
    explicit DspControlMidiRouter(const std::vector<parameterLayoutGroupData>& groups,
                                  juce::AudioProcessorValueTreeState& vts)
    {
        for (const auto& g : groups) {
            const int trackCh = midiChannelForSirenGroup(g.id);
            for (const auto& d : g.params) {
                const auto cls = ParameterIdGet::getClass(d->id);
                if (!isDspControlClass(cls))
                    continue;
                const auto* cc = std::get_if<CCParam>(&d->data);
                if (cc == nullptr || cc->midiCCNumber >= kCcCount)
                    continue;
                auto* p = vts.getParameter(ParameterIdGet::toJuceParameterId(g.id, d->id));
                if (p == nullptr)
                    continue;
                const auto idx = static_cast<size_t>(cc->midiCCNumber);
                if (d->id == ParameterId::ReverbEnable)
                    toggleCc[idx] = true;
                if (cls == ParameterClass::TrackControl) {
                    if (trackCh >= 1 && trackCh <= kSirenChannels)
                        track[static_cast<size_t>(trackCh)][idx] = p;
                } else {
                    mixer[idx] = p;
                }
            }
        }
    }

    // true when the message is a DSP Control CC this router owns.
    // Notes, siren CCs (volume, vibrato, …) and CC 121 stay with MidiIn.
    bool handle(const juce::MidiMessage& msg) const
    {
        if (!msg.isController())
            return false;
        const int ch = msg.getChannel();
        const int cc = msg.getControllerNumber();
        if (cc < 0 || cc >= kCcCount)
            return false;
        const auto idx = static_cast<size_t>(cc);
        const int value = msg.getControllerValue();

        if (ch >= 1 && ch <= kSirenChannels) {
            auto* p = track[static_cast<size_t>(ch)][idx];
            if (p == nullptr)
                return false;
            apply(p, value, toggleCc[idx]);
            return true;
        }

        if (ch != kMixerChannel)
            return false;
        auto* p = mixer[idx];
        if (p == nullptr)
            return false;
        apply(p, value, toggleCc[idx]);
        return true;
    }
};

#endif // COMPOSESIREN_DSPCONTROLMIDIROUTER_H
