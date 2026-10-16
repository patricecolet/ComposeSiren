//
// Extra MIDI-in: a virtual CoreMIDI/ALSA destination plus optional hardware.
//
// Destination name "SirenOrchestra: <role>" (macOS/Linux); further plugin
// instances number the SirenOrchestra prefix ("SirenOrchestra 2: <role>", …).
// Optional hardware MIDI device from Settings ("Virtual port" = virtual only).
//

#ifndef COMPOSESIREN_VIRTUALMIDIINPUT_H
#define COMPOSESIREN_VIRTUALMIDIINPUT_H

#include <juce_audio_devices/juce_audio_devices.h>

class VirtualMidiInput : public juce::MidiInputCallback
{
    juce::MidiMessageCollector collector;
    std::unique_ptr<juce::MidiInput> virtualIn;
    std::unique_ptr<juce::MidiInput> hardwareIn;
    juce::String hardwareName;
    juce::String role;

    static constexpr int kMaxVirtualPorts = 8;

    void openVirtual()
    {
        for (int n = 0; n < kMaxVirtualPorts && virtualIn == nullptr; ++n) {
            virtualIn = juce::MidiInput::createNewDevice(virtualDestinationName(role, n), this);
        }
        if (virtualIn != nullptr)
            virtualIn->start();
    }

public:
    /// instanceIndex 0: "SirenOrchestra: <role>"
    /// instanceIndex 1: "SirenOrchestra 2: <role>"
    static juce::String virtualDestinationName(const juce::String& role, int instanceIndex)
    {
        if (instanceIndex <= 0)
            return "SirenOrchestra: " + role;
        return "SirenOrchestra " + juce::String(instanceIndex + 1) + ": " + role;
    }

    explicit VirtualMidiInput(const juce::String& roleName)
        : role(roleName)
    {
        collector.reset(44100.0);
        openVirtual();
    }

    ~VirtualMidiInput() override
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

#endif // COMPOSESIREN_VIRTUALMIDIINPUT_H
