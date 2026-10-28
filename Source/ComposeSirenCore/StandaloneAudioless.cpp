// Standalone targets only (Source/CMakeLists.txt): see StandaloneAudio.h.

#include <juce_audio_plugin_client/detail/juce_IncludeModuleHeaders.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include "StandaloneAudio.h"

namespace {

using cs::standalone::AudioState;

// What AudioProcessorPlayer is prepared with while there is no audio device: two
// silent outputs at 48 kHz, 256 samples per block.
class SilentDevice final : public juce::AudioIODevice
{
public:
    static constexpr double sampleRate = 48000.0;
    static constexpr int blockSize = 256;

    SilentDevice() : juce::AudioIODevice("No audio output", "ComposeSiren") {}

    juce::StringArray getOutputChannelNames() override { return { "Left", "Right" }; }
    juce::StringArray getInputChannelNames() override { return {}; }
    juce::Array<double> getAvailableSampleRates() override { return { sampleRate }; }
    juce::Array<int> getAvailableBufferSizes() override { return { blockSize }; }
    int getDefaultBufferSize() override { return blockSize; }
    juce::String open(const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start(juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return blockSize; }
    double getCurrentSampleRate() override { return sampleRate; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override
    {
        juce::BigInteger channels;
        channels.setRange(0, 2, true);
        return channels;
    }
    juce::BigInteger getActiveInputChannels() const override { return {}; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }
};

// Watches the standalone's audio device. Without one, runs the player from its own
// thread at the pace of a real device; with one, stays out of the way.
class Audioless final : private juce::Timer, private juce::Thread
{
public:
    explicit Audioless(juce::StandalonePluginHolder& h)
        : juce::Thread("ComposeSiren: MIDI without audio"), holder(h)
    {
        timerCallback();
        startTimer(250);
    }

    ~Audioless() override
    {
        stopTimer();
        stopClock(holder.deviceManager.getCurrentAudioDevice() == nullptr);
        cs::standalone::setAudioState(AudioState::unknown);
    }

private:
    void timerCallback() override
    {
        const bool device = holder.deviceManager.getCurrentAudioDevice() != nullptr;
        if (device) {
            // The device prepared the player already: do not undo that.
            stopClock(false);
            cs::standalone::setAudioState(AudioState::device);
        } else {
            startClock();
            cs::standalone::setAudioState(AudioState::noDevice);
        }
    }

    void startClock()
    {
        if (clocking)
            return;
        holder.player.audioDeviceAboutToStart(&silent);
        clocking = true;
        startThread(juce::Thread::Priority::high);
    }

    void stopClock(bool releasePlayer)
    {
        if (!clocking)
            return;
        stopThread(2000);
        if (releasePlayer)
            holder.player.audioDeviceStopped();
        clocking = false;
    }

    void run() override
    {
        constexpr int block = SilentDevice::blockSize;
        constexpr double periodMs = 1000.0 * block / SilentDevice::sampleRate;
        juce::AudioBuffer<float> out(2, block);
        const juce::AudioIODeviceCallbackContext context{};
        auto next = juce::Time::getMillisecondCounterHiRes();

        while (!threadShouldExit()) {
            out.clear();
            holder.player.audioDeviceIOCallbackWithContext(
                nullptr, 0, out.getArrayOfWritePointers(), 2, block, context);

            next += periodMs;
            const auto now = juce::Time::getMillisecondCounterHiRes();
            if (next - now > 1.0)
                wait(static_cast<int>(next - now));
            else if (now - next > 100.0)
                next = now; // fell far behind (the machine slept): start again from now
        }
    }

    juce::StandalonePluginHolder& holder;
    SilentDevice silent;
    bool clocking = false;
};

struct InstallAudiolessKeeper
{
    InstallAudiolessKeeper()
    {
        cs::standalone::installKeeper([]() -> cs::standalone::Keeper {
            auto* holder = juce::StandalonePluginHolder::getInstance();
            if (holder == nullptr)
                return nullptr;
            return std::make_shared<Audioless>(*holder);
        });
    }
} installAudiolessKeeper;

} // namespace
