//
// Created by joseph larralde on 13/03/2026.
//

#ifndef COMPOSESIREN_MIDISCHEDULER_H
#define COMPOSESIREN_MIDISCHEDULER_H

#include <juce_audio_basics/juce_audio_basics.h>

class MidiScheduler {
    struct Event {
        juce::MidiMessage msg;
        int offset;
    };

    // maxEvents == 64 is not enough for SirenOrchestra, but 128
    // seems fine (all parameters are recalled on startup).
    // We could just keep the array solution with the increased
    // maxEvents value, but this feels wrong, so here is the
    // std::queue version (everything happens in the audio thread)
    // todo : estimate the overhead of using queue vs array

    // static constexpr int maxEvents = 128;
    // Event events[maxEvents] = {};
    // int count = 0;

    std::queue<Event> events;

public:
    void schedule(const juce::MidiMessage& msg, int offset) {
        // if (count >= maxEvents) { return; }
        // // x++ : eval, then increment => ok
        // events[count++] = { msg, offset };

        events.emplace(Event{msg, offset});
    }

    void flush(juce::MidiBuffer& buffer,
               AnyOrOneBasedMidiChannel outch = AnyOrOneBasedMidiChannel::any()) {
        // for (int i = 0; i < count; ++i) {
        //     if (!outch.isAny) {
        //         events[i].msg.setChannel(outch.channel.oneBased);
        //     }
        //     buffer.addEvent(events[i].msg, events[i].offset);
        // }
        // count = 0;

        while (!events.empty()) {
            auto e = events.front();
            if (!outch.isAny) {
                e.msg.setChannel(outch.channel.oneBased);
            }
            buffer.addEvent(e.msg, e.offset);
            events.pop();
        }
    }

    void reset() {
        // count = 0;

        std::queue<Event> empty;
        std::swap(events, empty);
    }
};

#endif //COMPOSESIREN_MIDISCHEDULER_H
