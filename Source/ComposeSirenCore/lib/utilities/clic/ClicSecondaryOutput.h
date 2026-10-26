// Where the click plays when it is not on the plugin's Clic bus: a second
// output device, so the metronome can go to a separate amp or headphones.
// COMPOSESIREN_CLIC. Message thread opens and closes the device; the audio
// thread only copies samples into a small ring.

#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <vector>

class ClicSecondaryOutput : private juce::AudioIODeviceCallback,
                            private juce::ChangeListener
{
public:
    ClicSecondaryOutput()
    {
        manager.addChangeListener(this);
    }

    ~ClicSecondaryOutput() override
    {
        manager.removeChangeListener(this);
        close();
    }

    /// Empty or "Main output": the Clic bus plays the click (no device here).
    /// Any other name: that output device plays it alone.
    void setDevice(const juce::String& name)
    {
        const auto want = (name.isEmpty() || name == "Main output") ? juce::String() : name;
        if (want == selected)
            return;
        close();
        selected = want;
        if (selected.isNotEmpty())
            open(selected);
    }

    bool isActive() const { return device != nullptr; }

    /// The click, already at the plugin's volume. Called from the audio thread.
    void push(const float* left, const float* right, int frames)
    {
        if (device == nullptr || frames <= 0)
            return;
        const int write = writeIndex.load(std::memory_order_relaxed);
        const int read = readIndex.load(std::memory_order_acquire);
        const int used = (write - read + capacity) % capacity;
        const int free = capacity - 1 - used;
        const int n = juce::jmin(frames, free);
        for (int i = 0; i < n; ++i) {
            const int at = (write + i) % capacity;
            buffer[static_cast<size_t>(at) * 2] = left[i];
            buffer[static_cast<size_t>(at) * 2 + 1] = right[i];
        }
        writeIndex.store((write + n) % capacity, std::memory_order_release);
    }

private:
    static constexpr int capacity = 8192; // samples per channel

    void open(const juce::String& name)
    {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        setup.outputDeviceName = name;
        setup.inputDeviceName = {};
        setup.sampleRate = 48000.0;
        setup.bufferSize = 0;
        const auto error = manager.initialise(0, 2, nullptr, true, {}, &setup);
        if (error.isNotEmpty()) {
            juce::ignoreUnused(error);
            return;
        }
        device = manager.getCurrentAudioDevice();
        if (device != nullptr) {
            buffer.assign(static_cast<size_t>(capacity) * 2, 0.0f);
            readIndex = writeIndex = 0;
            manager.addAudioCallback(this);
        }
    }

    void close()
    {
        if (device != nullptr) {
            manager.removeAudioCallback(this);
            manager.closeAudioDevice();
            device = nullptr;
        }
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        if (selected.isNotEmpty() && device == nullptr)
            open(selected);
    }

    void audioDeviceIOCallbackWithContext(const float* const* input,
                                          int numInput,
                                          float* const* output,
                                          int numOutput,
                                          int frames,
                                          const juce::AudioIODeviceCallbackContext&) override
    {
        juce::ignoreUnused(input, numInput);
        if (output == nullptr || numOutput <= 0 || frames <= 0)
            return;
        int read = readIndex.load(std::memory_order_relaxed);
        const int write = writeIndex.load(std::memory_order_acquire);
        for (int i = 0; i < frames; ++i) {
            float l = 0.0f, r = 0.0f;
            if (read != write) {
                l = buffer[static_cast<size_t>(read) * 2];
                r = buffer[static_cast<size_t>(read) * 2 + 1];
                read = (read + 1) % capacity;
            }
            output[0][i] = l;
            if (numOutput > 1)
                output[1][i] = r;
        }
        for (int c = 2; c < numOutput; ++c)
            juce::FloatVectorOperations::clear(output[c], frames);
        readIndex.store(read, std::memory_order_release);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice*) override {}
    void audioDeviceStopped() override {}

    juce::AudioDeviceManager manager;
    juce::AudioIODevice* device = nullptr;
    juce::String selected;
    std::vector<float> buffer;
    std::atomic<int> readIndex { 0 };
    std::atomic<int> writeIndex { 0 };
};
