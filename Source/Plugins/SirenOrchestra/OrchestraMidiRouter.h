//
// Created by joseph larralde on 27/04/2026.
//

#ifndef COMPOSESIREN_ORCHESTRAMIDIROUTER_H
#define COMPOSESIREN_ORCHESTRAMIDIROUTER_H

#include <PerSirenMidiBridges.h>

class OrchestraMidiRouter
{
    std::map<OneBasedMidiChannel, std::unique_ptr<PerSirenMidiBridges>> midiBridges;

public:
    OrchestraMidiRouter(const std::vector<parameterLayoutGroupData>& groups,
                        juce::AudioProcessorValueTreeState& vts,
                        juce::MidiKeyboardState& kbs)
    {
        for (auto& g : groups) {
            const auto& it = sirenIdByStrId.find(g.id);

            if (it != sirenIdByStrId.end()) {
                const auto& ch = sirenPropertiesById.at(it->second)->oneBasedMidiChannel;
                midiBridges[ch]
                    = std::make_unique<PerSirenMidiBridges>(g, vts, kbs);
                midiBridges[ch]->setDefaultInputChannel(ch);
                midiBridges[ch]->setAllowedInputChannel(AnyOrOneBasedMidiChannel::specific(ch));
                midiBridges[ch]->setMapChannel([](int c) { return c; });
            }
        }
    }

    ~OrchestraMidiRouter() = default;

#if COMPOSESIREN_RESETALLCONTROLLERS
    void setChannelModePolicy(cs::DspChannelModePolicy policy) {
        for (auto& bridges : midiBridges | std::ranges::views::values)
            bridges->setChannelModePolicy(policy);
    }
#endif

    void sendAllCurrentParameterValues() const {
        for (const auto& bridges : midiBridges | std::ranges::views::values) {
            bridges->sendAllCurrentParameterValues();
        }
    }

    void handleMessage(MidiScheduler& scheduler,
                       const juce::MidiMessage& msg,
                       int samplePosition) {
        // CC 121 on channel 16 is "Reset All": the same as a CC 121 on each
        // siren's own channel (that one reaches the DSP and the physical
        // sirens like any other message)
        if (msg.isControllerOfType(121) && msg.getChannel() == resetAllChannel) {
            resetSirens(scheduler, std::nullopt, msg.getControllerValue(), samplePosition);
            return;
        }
        OneBasedMidiChannel ch = {.oneBased=msg.getChannel()};
        const auto& it = midiBridges.find(ch);
        if (it != midiBridges.end()) {
            it->second->handleMessage(scheduler, msg, samplePosition);
        }
    }

    static constexpr int resetAllChannel = 16;

    // CC 121 as if it came in on `siren`'s channel, or on every siren's.
    void resetSirens(MidiScheduler& scheduler,
                     std::optional<OneBasedMidiChannel> siren,
                     int value,
                     int samplePosition) {
        for (const auto& [ch, bridges] : midiBridges) {
            if (!siren.has_value() || siren.value() == ch) {
                bridges->handleMessage(
                    scheduler,
                    juce::MidiMessage::controllerEvent(ch.oneBased, 121, value),
                    samplePosition);
            }
        }
    }

    void processBridges(MidiScheduler& scheduler, int numSamples) const {
        for (const auto& bridges : midiBridges | std::ranges::views::values) {
            bridges->processBridges(scheduler, numSamples);
        }
    }

};
#endif //COMPOSESIREN_ORCHESTRAMIDIROUTER_H
