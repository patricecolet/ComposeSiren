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

    void sendAllCurrentParameterValues() const {
        for (const auto& bridges : midiBridges | std::ranges::views::values) {
            bridges->sendAllCurrentParameterValues();
        }
    }

    void handleMessage(MidiScheduler& scheduler,
                       const juce::MidiMessage& msg,
                       int samplePosition) {
        OneBasedMidiChannel ch = {.oneBased=msg.getChannel()};
        const auto& it = midiBridges.find(ch);
        if (it != midiBridges.end()) {
            it->second->handleMessage(scheduler, msg, samplePosition);
        }
    }

    void processBridges(MidiScheduler& scheduler, int numSamples) const {
        for (const auto& bridges : midiBridges | std::ranges::views::values) {
            bridges->processBridges(scheduler, numSamples);
        }
    }

};
#endif //COMPOSESIREN_ORCHESTRAMIDIROUTER_H