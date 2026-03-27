//
// Created by joseph larralde on 04/03/2026.
//

#ifndef COMPOSESIREN_PARAMETERMIDIBRIDGE_H
#define COMPOSESIREN_PARAMETERMIDIBRIDGE_H

#include <juce_audio_processors/juce_audio_processors.h>
#include "atomicUtilities.h"
#include "MidiScheduler.h"
#include "lib/definitions/parameterDefinitions.h"

template<class T>
class ParameterMidiBridge : private juce::AudioProcessorParameter::Listener
{
    struct Event
    {
        int channel;
        float value;
        int samplePosition;
    };

    int ccNumber;
    juce::RangedAudioParameter& audioParam;
    bool useCCParamRange = false;
    OneBasedMidiChannel defaultInputChannel{};

    LatestValueMailbox<Event> mailbox;

    double sampleRate = 44100.0f;
    float emitRateHz = 50.0f;
    float emitPeriodMs = 1000.0f / emitRateHz;
    int emitPeriodSamples
        = std::max(static_cast<int>(emitPeriodMs * 0.001 * sampleRate), 1);
    int phaseCounter = 0;

    std::atomic<bool> updatingFromMidi { false };
    std::atomic<Event> pendingValue {{ defaultInputChannel.oneBased, 0, 0 }};
    // std::atomic<Event> pendingValue;
    std::atomic<bool> pending { false };

    // use these vars if fight between MIDI input and manual control
    // of parameter knobs becomes a problem :
    // bool gestureActive = false;
    // int gestureInactivitySamples = 0;

    std::function<int(int)> mapChannel = [](int ch) { return ch; };

public:
    ParameterMidiBridge(juce::RangedAudioParameter& p,
                        const OneBasedMidiChannel defaultInput,
                        const int cc = -1,
                        const bool useCCRange = false) :
    // ParameterMidiBridge(juce::RangedAudioParameter& p, bool useCCRange = false) :
        audioParam(p),
        defaultInputChannel(defaultInput),
        ccNumber(cc),
        useCCParamRange(useCCRange)
    {
        static_assert(std::is_same_v<CCParam, T> ||
                      std::is_same_v<PitchBendParam, T>);
        audioParam.addListener(this);
        pendingValue.store(
            { defaultInputChannel.oneBased, audioParam.getValue(), 0 },
            std::memory_order_relaxed
        );
    }

    ~ParameterMidiBridge() override
    {
        audioParam.removeListener(this);
    }

    // we want to limit the output MIDI rate to avoid flooding
    // but we don't want to be limited to the block-rate, so :
    void setSampleRate(double sr) {
        sampleRate = sr;
        emitPeriodSamples
            = std::max(static_cast<int>(emitPeriodMs * 0.001 * sampleRate), 1);
        phaseCounter = 0;
    }

    void setMapChannel(std::function<int(int)> fn)
    {
        mapChannel = std::move(fn);
    }

    void setDefaultInputChannel(OneBasedMidiChannel ch)
    {
        defaultInputChannel = ch;
    }

    void setPending()
    {
        // pendingValue.store(
        //     { defaultInputChannel.oneBased, audioParam.getValue(), 0 },
        //     std::memory_order_relaxed
        // );
        pending.store(true, std::memory_order_release);
    }

    template<class Q = T>
    std::enable_if<std::is_same_v<Q, CCParam>, void>::type
    handleIncomingEvent(const juce::MidiMessage& msg,
                        int samplePosition,
                        MidiScheduler& scheduler) {
        int channel = msg.getChannel();
        // int ccNumber = msg.getControllerNumber();
        int ccValue = msg.getControllerValue();
        float value = static_cast<float>(ccValue) / 127.0f;
        if (useCCParamRange) {
            value = audioParam.convertTo0to1(static_cast<float>(ccValue));
        }

        {
            ScopedGuard g(updatingFromMidi);
            audioParam.setValueNotifyingHost(value);
        }

        handleEventInternal({ channel, value, samplePosition }, scheduler);
    }

    template<class Q = T>
    std::enable_if<std::is_same_v<Q, PitchBendParam>, void>::type
    handleIncomingEvent(const juce::MidiMessage& msg,
                        int samplePosition,
                        MidiScheduler& scheduler) {
        int channel = msg.getChannel();
        int pbValue = msg.getPitchWheelValue();
        float value = static_cast<float>(pbValue) / 16383.0f;

        {
            ScopedGuard g(updatingFromMidi);
            // are the begin/end change gesture calls necessary ?
            // todo : check if this is already done by the attachments
            // audioParam.beginChangeGesture();
            audioParam.setValueNotifyingHost(value);
            // audioParam.endChangeGesture();
        }

        handleEventInternal({ channel, value, samplePosition }, scheduler);
    }
    // CC # / PitchBend aware parameter bridge versions can have
    // in-place specialized template method versions :
    // template<class Q = T>
    // use with :
    // std::enable_if<std::is_same_v<Q, CCParam>, void>::type
    // or :
    // std::enable_if<std::is_same_v<Q, PitchBendParam>, void>::type
    // or non-templated method version :
    // void
    // processBlock(MidiScheduler& out, int numSamples) {
    //     float value;
    //     if (mailbox.pop(value)) {
    //         emitParameterChange(value);
    //         // out.schedule(value, numSamples);
    //     }
    // }

    // dumbest version only dealing with normalized floats.
    // CC # / PitchBend business logic managed from
    // MidiRouter::processBridges(float &v)
    // todo : investigate why, for some reason, the max emitRateHz is not honored
    void processBlock(MidiScheduler& scheduler, const int numSamples) {
        Event newValue;

        if (mailbox.pop(newValue)) {
            pendingValue.store(newValue, std::memory_order_release);
            pending.store(true, std::memory_order_release);
            // pendingValue = newValue;
            // pending = true;
        }

        phaseCounter += numSamples;

        if (pending.load(std::memory_order_acquire) && phaseCounter >= emitPeriodSamples) {
        // if (pending && phaseCounter >= emitPeriodSamples) {
            // emit midi -> set vars right and return true
            Event e = pendingValue.load(std::memory_order_acquire);
            // Event e = pendingValue;
            int samplePosition = phaseCounter % numSamples;

            handleEventInternal({ defaultInputChannel.oneBased, e.value, samplePosition }, scheduler);
            phaseCounter = 0;
            pending.store(false, std::memory_order_release);
            // pending = false;
        }
    }

private:
    // ============================
    // PARAMETER LISTENER
    // ============================
    void parameterValueChanged(int, float newValue) override {
        // avoid MIDI echo
        if (updatingFromMidi.load(std::memory_order_relaxed)) { return; }
        mailbox.push({ defaultInputChannel.oneBased, newValue, 0 });
    }

    void parameterGestureChanged(int, bool) override {}

    // ============================
    // MIDI EMITTER
    // ============================

    template<class Q = T>
    std::enable_if<std::is_same_v<Q, CCParam>, void>::type
    handleEventInternal(Event e, MidiScheduler& scheduler)
    {
        int value = static_cast<int>(e.value * 127.0f);
        if (useCCParamRange) {
            value = audioParam.convertFrom0to1(e.value);
        }
        scheduler.schedule(
            juce::MidiMessage::controllerEvent(
                mapChannel(e.channel),
                ccNumber,
                value
            ),
            e.samplePosition
        );
    }

    template<class Q = T>
    std::enable_if<std::is_same_v<Q, PitchBendParam>, void>::type
    handleEventInternal(Event e, MidiScheduler& scheduler)
    {
        int value = static_cast<int>(e.value * 16383.0f);
        scheduler.schedule(
            juce::MidiMessage::pitchWheel(
                mapChannel(e.channel),
                value
            ),
            e.samplePosition
        );
    }

    // ============================
    // EMISSION
    // ============================
    /*
    void emitParameterChange(float value) {
        if (!gestureActive) {
            audioParam.beginChangeGesture();
            gestureActive = true;
        }

        audioParam.setValueNotifyingHost(value);
        gestureInactivitySamples = 0;
    }

    void emitMidi(float value, juce::MidiBuffer& midiOut) const {
        int ccValue = juce::jlimit(0, 127, static_cast<int>(value * 127.0f));

        midiOut.addEvent(
            juce::MidiMessage::controllerEvent(
                midiChannel,
                midiNumber,
                ccValue
            ),
            0
        );
    }

    void handleGestureTimeout(int numSamples) {
        if (gestureActive) {
            gestureInactivitySamples += numSamples;

            if (gestureInactivitySamples > emitIntervalSamples * 3) {
                audioParam.endChangeGesture();
                gestureActive = false;
            }
        }
    }
    //*/
};

#endif //COMPOSESIREN_PARAMETERMIDIBRIDGE_H
