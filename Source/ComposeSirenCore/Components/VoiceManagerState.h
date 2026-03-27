//
// Created by joseph larralde on 17/03/2026.
//

#ifndef COMPOSESIREN_VOICEMANAGERSTATE_H
#define COMPOSESIREN_VOICEMANAGERSTATE_H

#include "../lib/definitions/parameterDefinitions.h"

/**
 * Façade class for various listeners
 */

#include <atomic>
#include <map>
#include <set>
#include <juce_core/juce_core.h>
#include "uiParameterUtilities.h"

class VoiceManagerState
{
public:
    class Listener {
    public:
        enum class Key { category, midiInput, midiOutput };

        virtual ~Listener() = default;
        virtual void categoryChanged(sirenCategory) {}
        virtual void midiInputChanged(AnyOrOneBasedMidiChannel) {}
        virtual void midiOutputChanged(AnyOrOneBasedMidiChannel) {}
    };

    VoiceManagerState() :
        // this is where our default state is defined :
        c(sirenCategory::Alto),
        i(AnyOrOneBasedMidiChannel::any()),
        o(AnyOrOneBasedMidiChannel::any())
    {
        doForEachListenerKey([&](Listener::Key k) -> void {
            listeners.emplace(k, std::set<Listener*>());
        });
    }

    ~VoiceManagerState() {
        removeAllListeners();
    }

    void addListener(Listener* listener) {
        doForEachListenerKey([&](Listener::Key k) -> void {
            listeners.at(k).insert(listener);
        });
    }
    void addListener(Listener::Key k, Listener* listener) {
        listeners.at(k).insert(listener);
    }
    void removeListener(Listener* listener) {
        doForEachListenerKey([&](Listener::Key k) -> void {
            if (listeners.at(k).contains(listener)) {
                listeners.at(k).erase(listener);
            }
        });
    }
    void removeListener(Listener::Key k, Listener* listener) {
        listeners.at(k).erase(listener);
    }
    void removeAllListeners(Listener::Key k)    { listeners.at(k).clear(); }
    void removeAllListeners()                   { listeners.clear(); }

    [[nodiscard]] sirenCategory             getSirenCategory()  const { return c.load(); }
    [[nodiscard]] AnyOrOneBasedMidiChannel  getMidiInput()      const { return i.load(); }
    [[nodiscard]] AnyOrOneBasedMidiChannel  getMidiOutput()     const { return o.load(); }

    void setSirenCategory(const sirenCategory& category,
                          bool notify = false) {
        c.store(category);
        if (notify) { notifySirenCategory(); }
    }

    void setMidiInput(const AnyOrOneBasedMidiChannel& inch,
                      bool notify = false) {
        i.store(inch);
        if (notify) { notifyMidiInput(); }
    }
    void setMidiOutput(const AnyOrOneBasedMidiChannel& outch,
                      bool notify = false) {
        o.store(outch);
        if (notify) { notifyMidiOutput(); }
    }

    std::unique_ptr<juce::XmlElement> toXml() const {
        juce::ValueTree state = { "VoiceManagerState", {}, {
            { "VoiceManagerParameter", {
              { "id", "category" },
              { "value", menuIndexBySirenCategory.at(c.load()) }
            }},
            { "VoiceManagerParameter", {
              { "id", "midiInputChannel" },
              { "value", menuIndexByMidiChannel.at(i.load()) }
            }},
            { "VoiceManagerParameter", {
              { "id", "midiOutputChannel" },
              { "value", menuIndexByMidiChannel.at(o.load()) }
            }},
        }};
        return state.createXml();
    }

    void fromXml(const juce::XmlElement& xmlState) {
        juce::ValueTree state = juce::ValueTree::fromXml(xmlState);

        if (state.hasType("VoiceManagerState")) {
            for (int n = 0; n < state.getNumChildren(); ++n) {
                juce::ValueTree child = state.getChild(n);

                if (child.hasType("VoiceManagerParameter")
                    && child.hasProperty("id")
                    && child.hasProperty("value"))
                {
                    auto id = child.getProperty("id");
                    auto value = child.getProperty("value");

                    if (id == "category") {
                        setSirenCategory(sirenCategoryByMenuIndex.at(value), true);
                    } else if (id == "midiInputChannel") {
                        setMidiInput(midiChannelByMenuIndex.at(value), true);
                    } else if (id == "midiOutputChannel") {
                        setMidiOutput(midiChannelByMenuIndex.at(value), true);
                    }
                }
            }
        }
    }

private:
    std::map<Listener::Key, std::set<Listener*>> listeners;

    std::atomic<sirenCategory> c;
    std::atomic<AnyOrOneBasedMidiChannel> i;
    std::atomic<AnyOrOneBasedMidiChannel> o;

    static void doForEachListenerKey(std::function<void(Listener::Key)> fn) {
        std::array keys = {
            Listener::Key::category,
            Listener::Key::midiInput,
            Listener::Key::midiOutput
        };

        for (auto k : keys) { fn(k); }
    }

    void notifySirenCategory() const {
        for (const auto listener : listeners.at(Listener::Key::category)) {
            listener->categoryChanged(c.load());
        }
    }
    void notifyMidiInput() const {
        for (const auto listener : listeners.at(Listener::Key::midiInput)) {
            listener->midiInputChanged(i.load());
        }
    }
    void notifyMidiOutput() const {
        for (const auto listener : listeners.at(Listener::Key::midiOutput)) {
            listener->midiOutputChanged(o.load());
        }
    }
};

#endif //COMPOSESIREN_VOICEMANAGERSTATE_H