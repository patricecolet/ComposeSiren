//
// Extra MIDI-in mapping for mixer DSP parameters (reverb, per-siren pan/gain,
// master volume). CC numbers and classes come from the generated UiMetadata
// (composesiren-parameters.csv). Channel 1–7 are the sirens; channel 16 is
// the orchestra mixer bus (master and reverb).
//
// This is not MidiIn (the siren firmware CC table: volume, vibrato, notes).
// Messages handled here only move APVTS parameters; ParameterBridges already
// drive the DSP. Nothing is scheduled onto the siren MIDI output or the park.
//

#ifndef COMPOSESIREN_DSPMIDIROUTER_H
#define COMPOSESIREN_DSPMIDIROUTER_H

#include <array>
#include <string>

#include <juce_audio_processors/juce_audio_processors.h>

#include <apvtsUtilities.h>
#include <lib/definitions/generated/UiMetadata.h>
#include <lib/definitions/parameterDefinitions.h>
#include <lib/definitions/sirenProperties.h>

class DspMidiRouter
{
    static constexpr int kSirenChannels = 7;
    static constexpr int kMixerChannel = 16;
    static constexpr int kCcCount = 128;

    using CcParams = std::array<juce::RangedAudioParameter*, kCcCount>;
    std::array<CcParams, kSirenChannels + 1> track {};
    CcParams mixer {};
    std::array<bool, kCcCount> toggleCc {};

    static juce::RangedAudioParameter* param(juce::AudioProcessorValueTreeState& vts,
                                             const std::string& group,
                                             std::string_view codeName)
    {
        const auto it = parameterIdByCodeName.find(std::string(codeName));
        if (it == parameterIdByCodeName.end())
            return nullptr;
        return vts.getParameter(ParameterIdGet::toJuceParameterId(group, it->second));
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

    static void bind(CcParams& dest,
                     juce::AudioProcessorValueTreeState& vts,
                     const std::string& group,
                     const mecaviv::metadata::ui::Parameter& p)
    {
        if (p.midi != mecaviv::metadata::ui::Midi::Cc || p.cc < 0 || p.cc >= kCcCount)
            return;
        dest[static_cast<size_t>(p.cc)] = param(vts, group, p.codeName);
    }

public:
    explicit DspMidiRouter(juce::AudioProcessorValueTreeState& vts)
    {
        namespace ui = mecaviv::metadata::ui;
        for (const auto& p : ui::parameters) {
            if (p.midi != ui::Midi::Cc || p.cc < 0 || p.cc >= kCcCount)
                continue;
            if (p.widget == ui::Widget::Toggle)
                toggleCc[static_cast<size_t>(p.cc)] = true;
            if (p.parameterClass == ui::ParameterClass::Track) {
                for (const auto sid : allSirenIds) {
                    const int ch = sirenPropertiesById.at(sid)->oneBasedMidiChannel.oneBased;
                    if (ch < 1 || ch > kSirenChannels)
                        continue;
                    bind(track[static_cast<size_t>(ch)], vts, sirenStrIdById.at(sid), p);
                }
            } else if (p.parameterClass == ui::ParameterClass::Reverb) {
                bind(mixer, vts, "R", p);
            } else if (p.parameterClass == ui::ParameterClass::Master) {
                bind(mixer, vts, "M", p);
            }
        }
    }

    // true when the message is a mixer DSP CC this router owns.
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

#endif // COMPOSESIREN_DSPMIDIROUTER_H
