//
// Created by joseph larralde on 24/04/2026.
//

#ifndef COMPOSESIREN_PER_SIREN_MIDI_BRIDGES_H
#define COMPOSESIREN_PER_SIREN_MIDI_BRIDGES_H

#include <ranges>

#include "KeyboardMidiBridge.h"
#include "ParameterMidiBridge.h"
#include "MidiScheduler.h"
#include "apvtsUtilities.h"

class PerSirenMidiBridges
{
    OneBasedMidiChannel defaultInputChannel{.oneBased=1};
    AnyOrOneBasedMidiChannel allowedInputChannel{AnyOrOneBasedMidiChannel::any()};

    std::unique_ptr<KeyboardMidiBridge> noteBridge;
    std::unique_ptr<ParameterMidiBridge<PitchBendParam>> pitchBendBridge;
    std::map<int, std::unique_ptr<ParameterMidiBridge<CCParam>>> bridgeByCCNumber;

    static juce::RangedAudioParameter*
    getRangedParameterFromVTS(const std::string& groupId,
                              const ParameterId& parameterId,
                              juce::AudioProcessorValueTreeState& vts) {
        const auto juceId = ParameterIdGet::toJuceParameterId(groupId, parameterId);
        return vts.getParameter(juceId);
    }

public:
    PerSirenMidiBridges(const parameterLayoutGroupData& layoutData,
                        juce::AudioProcessorValueTreeState& vts,
                        juce::MidiKeyboardState& kbs)
    {
        noteBridge = std::make_unique<KeyboardMidiBridge>(layoutData,
                                                          vts,
                                                          kbs,
                                                          allowedInputChannel);
        for (const auto& d : layoutData.params) {
            auto p = getRangedParameterFromVTS(layoutData.id, d->id, vts);
            // all parameters should be found by juce parameter id
            if (p == nullptr) assert(false);

            if (const auto pb = std::get_if<PitchBendParam>(&d->data)) {
                pitchBendBridge
                    = std::make_unique<ParameterMidiBridge<PitchBendParam>>(
                        *p,
                        defaultInputChannel
                    );
            } else if (const auto cc = std::get_if<CCParam>(&d->data)) {
                int ccNumber = cc->midiCCNumber;
                bridgeByCCNumber[cc->midiCCNumber]
                    = std::make_unique<ParameterMidiBridge<CCParam>>(
                        *p,
                        defaultInputChannel,
                        ccNumber,
                        d->id == ParameterId::PitchBendRange
                        || d->id == ParameterId::AllNoteOff
                        || d->id == ParameterId::AllSoundOff
                        || d->id == ParameterId::ResetAllController
                    );
            }
        }
    }
    ~PerSirenMidiBridges() = default;

    void setAllowedInputChannel(AnyOrOneBasedMidiChannel channel) {
        allowedInputChannel = channel;
        noteBridge->setAllowedInputChannel(channel);
    }

    void setDefaultInputChannel(OneBasedMidiChannel channel) {
        defaultInputChannel = channel;
        // todo : remove setDefaultInputChannel method from KeyboardMidiBridge
        //  because default channel is defined at the keyboard component level
        // noteBridge->setDefaultInputChannel(channel);
        for (auto& bridge : bridgeByCCNumber | std::ranges::views::values) {
            bridge->setDefaultInputChannel(channel);
        }
        pitchBendBridge->setDefaultInputChannel(channel);
    }

    void setMapChannel(const std::function<int(int)>& fn) {
        noteBridge->setMapChannel(fn);
        for (auto& bridge : bridgeByCCNumber | std::ranges::views::values) {
            bridge->setMapChannel(fn);
        }
        pitchBendBridge->setMapChannel(fn);
    }

    void sendAllCurrentParameterValues() {
        for (const auto& [cc, bridge] : bridgeByCCNumber) {
            // take care not to reset the state with these cc parameters
            // actually only 121 (reset all controller) is handled by MidiIn
            // but neither will we ever want the two others to interfere
            // so we ignore them here :
            if (cc != 120 && cc != 121 && cc != 123) {
                bridge->setPending();
            }
        }
        pitchBendBridge->setPending();
    }

    void handleMessage(MidiScheduler& scheduler,
                       const juce::MidiMessage& msg,
                       int samplePosition)
    {
        if (msg.isController() && bridgeByCCNumber.contains(msg.getControllerNumber())) {
            bridgeByCCNumber[msg.getControllerNumber()]->handleIncomingEvent(msg, samplePosition, scheduler);
        } else if (msg.isPitchWheel()) {
            pitchBendBridge->handleIncomingEvent(msg, samplePosition, scheduler);
        } else if (msg.isNoteOnOrOff()) {
            noteBridge->handleIncomingNoteEvent(msg, samplePosition, scheduler);
        }
    }

    void processBridges(MidiScheduler& scheduler, int numSamples)
    {
        for (const auto& bridge : bridgeByCCNumber | std::ranges::views::values) {
            bridge->processBlock(scheduler, numSamples);
        }

        pitchBendBridge->processBlock(scheduler, numSamples);
        noteBridge->processBlock(scheduler, numSamples);
    }
};

#endif //COMPOSESIREN_PER_SIREN_MIDI_BRIDGES_H