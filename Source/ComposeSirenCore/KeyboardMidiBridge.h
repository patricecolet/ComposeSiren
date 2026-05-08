//
// Created by joseph larralde on 17/03/2026.
//

#ifndef COMPOSESIREN_KEYBOARDMIDIBRIDGE_H
#define COMPOSESIREN_KEYBOARDMIDIBRIDGE_H

// #include <juce_audio_devices/juce_audio_devices.h>
#include "atomicUtilities.h"
#include "apvtsUtilities.h"
#include "MidiScheduler.h"

class KeyboardMidiBridge : public juce::AudioProcessorParameter::Listener,
                           public juce::MidiKeyboardState::Listener
{
    struct NoteEvent {
        int channel;
        int note;
        float velocity;
        bool isNoteOn;
        int samplePosition;
    };

    struct ActiveNote {
        int transformedChannel;
        int transformedNote;
    };

    juce::MidiKeyboardState& keyboardState;
    juce::RangedAudioParameter* transposeParam;
    AnyOrOneBasedMidiChannel allowedInputChannel;

    std::atomic<bool> updatingFromMidi { false };

    // Event queue (GUI -> audio)
    juce::SpinLock queueLock{};
    std::vector<NoteEvent> eventQueue;
    std::vector<NoteEvent> tempQueue; // audio-thread local

    // Active note tracking
    std::map<std::pair<int,int>, ActiveNote> activeNotes{};

    // Transform
    int transpose = 0;
    std::function<int(int)> mapChannel = [](int ch) { return ch; };

public:
    KeyboardMidiBridge(const parameterLayoutGroupData& layoutData,
                       juce::AudioProcessorValueTreeState& vts,
                       juce::MidiKeyboardState& s,
                       AnyOrOneBasedMidiChannel allowedInput) :
        keyboardState(s),
        allowedInputChannel(allowedInput)
    {
        transposeParam = vts.getParameter(
            ParameterIdGet::toJuceParameterId(
            layoutData.id,
            ParameterId::Transpose
        ));
        transposeParam->addListener(this);
        keyboardState.addListener(this);
    }

    ~KeyboardMidiBridge() override
    {
        transposeParam->removeListener(this);
        keyboardState.removeListener(this);
    }

    void setMapChannel(std::function<int(int)> fn)
    {
        mapChannel = std::move(fn);
    }

    void setAllowedInputChannel(AnyOrOneBasedMidiChannel ch)
    {
        allowedInputChannel = ch;
    }

    // bool processBlock(float& v, int& samplePosition, int numSamples);

    // todo : improve output rate using numSamples and phase accumulation
    //  as in ParameterMidiBridge::processBlock ?
    void processBlock(MidiScheduler& scheduler, int numSamples)
    {
        // get pending events and clear queue by swapping
        tempQueue.clear();
        {
            juce::SpinLock::ScopedLockType lock(queueLock);
            std::swap(tempQueue, eventQueue);
        }

        for (const auto& e : tempQueue) {
            if (e.isNoteOn) {
                handleNoteOnInternal(e, scheduler);
            } else {
                handleNoteOffInternal(e, scheduler);
            }
        }
    }

    void handleIncomingNoteEvent(const juce::MidiMessage& msg,
                                 int samplePosition,
                                 MidiScheduler& scheduler)
    {
        int channel = msg.getChannel();
        int note = msg.getNoteNumber();
        float velocity = msg.getFloatVelocity();
        bool isOn = msg.isNoteOn();

        if (isOn) {
            {
                ScopedGuard g(updatingFromMidi);
                keyboardState.noteOn(channel, note, velocity);
            }

            handleNoteOnInternal(
                { channel, note, velocity, true, samplePosition },
                scheduler
            );
        } else {
            {
                ScopedGuard g(updatingFromMidi);
                keyboardState.noteOff(channel, note, velocity);
            }

            handleNoteOffInternal(
                { channel, note, velocity, false, samplePosition },
                scheduler
            );
        }
    }

private:
    // TRANSPOSE PARAM LISTENER ================================================
    void parameterValueChanged(int, float newValue) override {
        transpose = static_cast<int>(transposeParam->convertFrom0to1(newValue));
    }

    void parameterGestureChanged(int, bool isGesture) override {}

    // KEYBOARD STATE LISTENER =================================================

    // NB : we don't define the GUI Keyboard channel in handleNoteOn and
    // HandleNoteOff. We do it directly in the KeyboardComponent so it is
    // transmitted to the KeyboardState, then optionally filtered here.

    void handleNoteOn(juce::MidiKeyboardState* source,
                      int channel,
                      int note,
                      float velocity) override
    {
        // avoid MIDI echo
        if (updatingFromMidi.load(std::memory_order_relaxed)) { return; }
        // filter input channel :
        // allow any  channel, or only a specific one if there are several
        // KeyboardMidiBridges listening to the same KeyboardState
        if (allowedInputChannel != AnyOrOneBasedMidiChannel::any() &&
            allowedInputChannel.channel.oneBased != channel) { return; }
        pushEvent({ channel, note, velocity, true, 0 });
    }

    void handleNoteOff(juce::MidiKeyboardState* source,
                       int channel,
                       int note,
                       float velocity) override
    {
        // avoid MIDI echo
        if (updatingFromMidi.load(std::memory_order_relaxed)) { return; }
        // filter input channel
        if (allowedInputChannel != AnyOrOneBasedMidiChannel::any() &&
            allowedInputChannel.channel.oneBased != channel) { return; }
        pushEvent({ channel, note, velocity, false, 0 });
    }

    void pushEvent(const NoteEvent& e)
    {
        juce::SpinLock::ScopedLockType lock(queueLock);
        eventQueue.push_back(e);
    }

    // AUDIO THREAD LOGIC ======================================================

    void handleNoteOnInternal(const NoteEvent& e, MidiScheduler& scheduler)
    {
        const auto key = std::make_pair(e.channel, e.note);

        const ActiveNote n {
            .transformedChannel = mapChannel(e.channel),
            .transformedNote = juce::jlimit(0, 127, e.note + transpose)
        };

        activeNotes[key] = n;

        scheduler.schedule(
            juce::MidiMessage::noteOn(
                n.transformedChannel,
                n.transformedNote,
                e.velocity
            ),
            e.samplePosition
        );
    }

    void handleNoteOffInternal(const NoteEvent& e, MidiScheduler& scheduler)
    {
        const auto key = std::make_pair(e.channel, e.note);

        auto it = activeNotes.find(key);
        if (it == activeNotes.end())
            return; // defensive

        const auto& n = it->second;

        scheduler.schedule(
            juce::MidiMessage::noteOff(
                n.transformedChannel,
                n.transformedNote
            ),
            e.samplePosition
        );

        activeNotes.erase(it);
    }
};

#endif // COMPOSESIREN_KEYBOARDMIDIBRIDGE_H
