//
// Created by joseph larralde on 17/03/2026.
//

#ifndef COMPOSESIREN_VOICEMANAGERSTATE_H
#define COMPOSESIREN_VOICEMANAGERSTATE_H

#include "../lib/definitions/parameterDefinitions.h"

/**
 * Façade class for various listeners
 */

class VoiceManagerState
{
public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void categoryChanged(sirenCategory) {}
        virtual void midiInputChanged(AnyOrOneBasedMidiChannel) {}
        virtual void midiOutputChanged(AnyOrOneBasedMidiChannel) {}
    };

    VoiceManagerState() :
        c(sirenCategory::Alto),
        i(AnyMidiChannel{}),
        o(AnyMidiChannel{})
    {}

    ~VoiceManagerState() = default;

    void addListener(Listener* listener)    { listeners.insert(listener); }
    void removeListener(Listener* listener) { listeners.erase(listener); }

    [[nodiscard]] const sirenCategory& getSirenCategory() const { return c; }
    [[nodiscard]] const AnyOrOneBasedMidiChannel& getMidiInput() const { return i; }
    [[nodiscard]] const AnyOrOneBasedMidiChannel& getMidiOutput() const { return o; }

    void setSirenCategory(sirenCategory category) {
        c = category;
        for (const auto listener : listeners) {
            listener->categoryChanged(c);
        }
    }

    void setMidiInput(AnyOrOneBasedMidiChannel inch) {
        i = inch;
        for (const auto listener : listeners) {
            listener->midiInputChanged(i);
        }
    }
    void setMidiOutput(AnyOrOneBasedMidiChannel outch) {
        o = outch;
        for (const auto listener : listeners) {
            listener->midiOutputChanged(o);
        }
    }

private:
    std::set<Listener*> listeners;
    sirenCategory c;
    AnyOrOneBasedMidiChannel i;
    AnyOrOneBasedMidiChannel o;
};

#endif //COMPOSESIREN_VOICEMANAGERSTATE_H