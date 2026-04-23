//
// Created by joseph larralde on 11/03/2026.
//

#ifndef COMPOSESIREN_MIDIROUTER_H
#define COMPOSESIREN_MIDIROUTER_H

// #include <juce_audio_processors/juce_audio_processors.h>
#include <ranges>

#include "KeyboardMidiBridge.h"
#include "ParameterMidiBridge.h"
#include "MidiScheduler.h"
#include "apvtsUtilities.h"

// per Siren/channel/parameterGroup MIDI messages dispatcher
class MidiRouter
{
    AnyOrOneBasedMidiChannel inch;
    AnyOrOneBasedMidiChannel outch;
    OneBasedMidiChannel defaultInputChannel = {.oneBased=1};

    // double sampleRate = 44100.0;
    // float emitRateHz = 50.0f;
    // float emitPeriodMs = 1000.0f / emitRateHz;
    // int emitPeriodSamples
    //     = std::max(static_cast<int>(emitPeriodMs * 0.001 * sampleRate), 1);
    // int phaseCounter = 0;

    // bool shouldFlush = false;
    std::unique_ptr<KeyboardMidiBridge> noteBridge;

    // CC # / PitchBend aware parameter bridge versions :
    std::unique_ptr<ParameterMidiBridge<PitchBendParam>> pitchBendBridge;
    std::map<int, std::unique_ptr<ParameterMidiBridge<CCParam>>> bridgeByCCNumber;

    // std::unique_ptr<ParameterMidiBridge> pitchBendBridge;
    // std::map<int, std::unique_ptr<ParameterMidiBridge>> bridgeByCCNumber;

    static juce::RangedAudioParameter*
    getRangedParameterFromVTS(const std::string& groupId,
                              const ParameterId& parameterId,
                              juce::AudioProcessorValueTreeState& vts) {
        const auto juceId = ParameterIdGet::toJuceParameterId(groupId, parameterId);
        return vts.getParameter(juceId);
    }

public:
    MidiRouter(const parameterLayoutGroupData& layoutData,
               juce::AudioProcessorValueTreeState& vts,
               juce::MidiKeyboardState& kbs) :
        inch(AnyOrOneBasedMidiChannel::any()),
        outch(AnyOrOneBasedMidiChannel::any())
    {
        noteBridge = std::make_unique<KeyboardMidiBridge>(layoutData,
                                                          vts,
                                                          kbs,
                                                          defaultInputChannel);

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

    ~MidiRouter() = default;

    [[nodiscard]] AnyOrOneBasedMidiChannel getInputMidiChannel() const {
        return inch;
    }

    void setInputMidiChannel(const AnyOrOneBasedMidiChannel& ch) {
        inch = ch;
        // flushPendingNoteEvents();
    }

    [[nodiscard]] AnyOrOneBasedMidiChannel getOutputMidiChannel() const {
        return outch;
    }

    void setOutputMidiChannel(const AnyOrOneBasedMidiChannel& ch) {
        // flushPendingNoteEvents();

        std::function<int(int)> mapChannel = [](int c) { return c; };
        if (!ch.isAny) {
            mapChannel = [val = ch.channel.oneBased](int) { return val; };
        }

        for (const auto& bridge : bridgeByCCNumber | std::views::values) {
            bridge->setMapChannel(mapChannel);
        }
        pitchBendBridge->setMapChannel(mapChannel);
        noteBridge->setMapChannel(mapChannel);

        // sendAllCurrentParameterValues();
    }

    void sendAllCurrentParameterValues() {
        for (const auto& [cc, bridge] : bridgeByCCNumber) {
            // take care not to reset the state with these cc parameters
            // actually only 121 (reset all controller) is handled by MidiIn
            // but we'll never want the two others to interfere so we ignore
            // them here :
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
        if (!inch.isAny && msg.getChannel() != inch.channel.oneBased) {
            return;
        }

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
        for (const auto& bridge : bridgeByCCNumber | std::views::values) {
            bridge->processBlock(scheduler, numSamples);
        }

        pitchBendBridge->processBlock(scheduler, numSamples);
        noteBridge->processBlock(scheduler, numSamples);
    }

    void flushPendingNoteEvents() {
        // todo
    }
};

#endif //COMPOSESIREN_MIDIROUTER_H
