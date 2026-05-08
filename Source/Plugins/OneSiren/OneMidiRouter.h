//
// Created by joseph larralde on 27/04/2026.
//

#ifndef COMPOSESIREN_ONEMIDIROUTER_H
#define COMPOSESIREN_ONEMIDIROUTER_H

#include <PerSirenMidiBridges.h>

class OneMidiRouter
{
    AnyOrOneBasedMidiChannel inch;
    std::unique_ptr<PerSirenMidiBridges> midiBridges;

public:
    OneMidiRouter(const parameterLayoutGroupData& group,
                  juce::AudioProcessorValueTreeState& vts,
                  juce::MidiKeyboardState& kbs) :
        inch(AnyOrOneBasedMidiChannel::any())
    {
        midiBridges = std::make_unique<PerSirenMidiBridges>(group, vts, kbs);
    }
    ~OneMidiRouter() = default;

    void setInputMidiChannel(const AnyOrOneBasedMidiChannel& ch) {
        inch = ch;
        OneBasedMidiChannel defaultInputChannel = inch.isAny
                                                  ? OneBasedMidiChannel{.oneBased=1}
                                                  : inch.channel;

        midiBridges->setAllowedInputChannel(AnyOrOneBasedMidiChannel::any());
        midiBridges->setDefaultInputChannel(defaultInputChannel);
        // flushPendingNoteEvents();
    }

    void setOutputMidiChannel(const AnyOrOneBasedMidiChannel& ch) const {
        // flushPendingNoteEvents();
        std::function<int(int)> mapChannel = [](int c) { return c; };

        if (!ch.isAny) {
            mapChannel = [val = ch.channel.oneBased](int) { return val; };
        }

        midiBridges->setMapChannel(mapChannel);
        // sendAllCurrentParameterValues();
    }

    void sendAllCurrentParameterValues() const {
        midiBridges->sendAllCurrentParameterValues();
    }

    void handleMessage(MidiScheduler& scheduler,
                       const juce::MidiMessage& msg,
                       int samplePosition) const {
        if (!inch.isAny && msg.getChannel() != inch.channel.oneBased) {
            return;
        }

        midiBridges->handleMessage(scheduler, msg, samplePosition);
    }

    void processBridges(MidiScheduler& scheduler, int numSamples) const {
        midiBridges->processBridges(scheduler, numSamples);
    }
};
#endif //COMPOSESIREN_ONEMIDIROUTER_H