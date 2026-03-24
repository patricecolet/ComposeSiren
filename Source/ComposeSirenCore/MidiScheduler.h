//
// Created by joseph larralde on 13/03/2026.
//

#ifndef COMPOSESIREN_MIDISCHEDULER_H
#define COMPOSESIREN_MIDISCHEDULER_H

#include <juce_audio_basics/juce_audio_basics.h>

class MidiScheduler {
public:
    void schedule(const juce::MidiMessage& msg, int offset) {
        if (count >= maxEvents) { return; }
        // x++ : eval, then increment => ok
        events[count++] = { msg, offset };
    }

    void flush(juce::MidiBuffer& buffer, AnyOrOneBasedMidiChannel outch = AnyMidiChannel{}) {
        if (auto* oc = std::get_if<OneBasedMidiChannel>(&outch)) {
            for (int i = 0; i < count; ++i) {
                events[i].msg.setChannel(oc->oneBased);
                buffer.addEvent(events[i].msg, events[i].offset);
            }
        } else {
            for (int i = 0; i < count; ++i) {
                buffer.addEvent(events[i].msg, events[i].offset);
            }
        }
        count = 0;
    }

    void reset() {
        count = 0;
    }

private:
    struct Event {
        juce::MidiMessage msg;
        int offset;
    };

    static constexpr int maxEvents = 64;
    Event events[maxEvents] = {};
    int count = 0;
};

#endif //COMPOSESIREN_MIDISCHEDULER_H