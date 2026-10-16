//
// Extra MIDI-in port for mixer DSP controls (DspMidiRouter).
//
// A virtual CoreMIDI/ALSA destination named "SirenOrchestra DSP" (macOS/Linux)
// plus an optional hardware MIDI device from Settings. Incoming messages never
// enter MidiIn or OrchestraMidiRouter; the processor drains them into
// DspMidiRouter only.
//

#ifndef COMPOSESIREN_DSPMIDIINPUT_H
#define COMPOSESIREN_DSPMIDIINPUT_H

#include <juce_audio_devices/juce_audio_devices.h>

class DspMidiInput : public juce::MidiInputCallback
{
    juce::MidiMessageCollector collector;
    std::unique_ptr<juce::MidiInput> virtualIn;
    std::unique_ptr<juce::MidiInput> hardwareIn;
    juce::String hardwareName;

    static constexpr const char* kVirtualName = "SirenOrchestra DSP";

    void openVirtual()
    {
        for (int n = 0; n < 8 && virtualIn == nullptr; ++n) {
            const juce::String name = n == 0
                                          ? juce::String(kVirtualName)
                                          : juce::String(kVirtualName) + " " + juce::String(n + 1);
            virtualIn = juce::MidiInput::createNewDevice(name, this);
        }
        if (virtualIn != nullptr)
            virtualIn->start();
    }

public:
    DspMidiInput()
    {
        collector.reset(44100.0);
        openVirtual();
    }

    ~DspMidiInput() override
    {
        if (hardwareIn != nullptr)
            hardwareIn->stop();
        if (virtualIn != nullptr)
            virtualIn->stop();
    }

    void setSampleRate(double sr)
    {
        collector.reset(sr > 0.0 ? sr : 44100.0);
    }

    /// Empty or "Virtual port": the virtual destination only.
    /// Any other name: also open that MIDI input device.
    void setHardwareDevice(const juce::String& name)
    {
        const auto want = (name.isEmpty() || name == "Virtual port") ? juce::String() : name;
        if (want == hardwareName)
            return;
        if (hardwareIn != nullptr) {
            hardwareIn->stop();
            hardwareIn.reset();
        }
        hardwareName = want;
        if (hardwareName.isEmpty())
            return;
        for (const auto& d : juce::MidiInput::getAvailableDevices()) {
            if (d.name != hardwareName)
                continue;
            hardwareIn = juce::MidiInput::openDevice(d.identifier, this);
            if (hardwareIn != nullptr)
                hardwareIn->start();
            break;
        }
    }

    juce::String virtualPortName() const
    {
        return virtualIn != nullptr ? virtualIn->getName() : juce::String();
    }

    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override
    {
        collector.addMessageToQueue(message);
    }

    void drain(juce::MidiBuffer& dest, int numSamples)
    {
        collector.removeNextBlockOfMessages(dest, juce::jmax(1, numSamples));
    }
};

#endif // COMPOSESIREN_DSPMIDIINPUT_H
