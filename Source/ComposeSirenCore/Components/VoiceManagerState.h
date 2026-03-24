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

    VoiceManagerState() = default;
    ~VoiceManagerState() = default;

    void addListener(Listener* listener)    { listeners.insert(listener); }
    void removeListener(Listener* listener) { listeners.erase(listener); }

    void setSirenCategory(sirenCategory category) {
        for (const auto listener : listeners) {
            listener->categoryChanged(category);
        }
    }

    void setMidiInput(AnyOrOneBasedMidiChannel inch) {
        for (const auto listener : listeners) {
            listener->midiInputChanged(inch);
        }
    }
    void setMidiOutput(AnyOrOneBasedMidiChannel outch) {
        for (const auto listener : listeners) {
            listener->midiOutputChanged(outch);
        }
    }

private:
    std::set<Listener*> listeners;
};

#endif //COMPOSESIREN_VOICEMANAGERSTATE_H