//
// Created by joseph larralde on 07/02/2026.
//

#ifndef COMPOSESIREN_SIREN_VOICE_H
#define COMPOSESIREN_SIREN_VOICE_H

#include <set>
#include <optional>
#include "../definitions/parameterDefinitions.h"
#include "../dsp/MidiIn.h"
#include "../dsp/Sirene.h"

#ifndef COMPOSESIREN_RUST_DSP
#define COMPOSESIREN_RUST_DSP 0
#endif
#if COMPOSESIREN_RUST_DSP
#include <array>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include "DspChoice.h"
#include "siren_dsp_plugin.h"
#endif

// VOICE UNIT BASED ON FILE RESOURCES SET
// TO BE SWAPPED WITH ANOTHER INSTANCE

class SirenVoiceUnit {
    sirenId id;
    std::shared_ptr<const sirenData> data;
    int maxNote; // as defined in velocityRanges
    std::unique_ptr<MidiIn> midiIn;
    std::unique_ptr<Sirene> siren; // the C++ synthesis (null when a Rust one plays)
#if COMPOSESIREN_RUST_DSP
    // the Rust synthesis (dsp.variant other than "cpp"), driven exactly like `siren`
    struct RustSirenDeleter { void operator()(CsrSiren* s) const { csr_siren_free(s); } };
    std::unique_ptr<CsrSiren, RustSirenDeleter> rustSiren;
#endif
    double sampleRate;
    int sampleCountForMidiInTimer;
    int setNoteCallbackPeriodSamples;
    int setNoteSampleCounter;

    std::atomic<bool> isNoteOn { false };
    bool ino = false;
    std::atomic<float> currentPitch { 0.0f };
    float cp = 0.0f;

public:
#if COMPOSESIREN_RUST_DSP
    // `choice`: which synthesis plays, on which data set (DspChoice.h); an unknown or failing
    // Rust variant falls back to the C++ one, with a message on stderr
    SirenVoiceUnit(sirenId id, const std::string& resourcesPath, const cs::DspChoice& choice = cs::currentDspChoice());
    bool isRust() const { return rustSiren != nullptr; }
    // audio thread: a note is held (what the voice waits for before a DSP swap)
    bool isHeld() const { return ino; }
    // set before the unit is handed to the audio thread: take its place at once (a siren
    // change), not at the next note-on with no note held (a DSP change)
    bool adoptAtOnce = false;
#else
    SirenVoiceUnit(sirenId id, const std::string& resourcesPath);
#endif
    ~SirenVoiceUnit() = default;

    // this method should check if siren is nullptr
    // if it's not, instantiate a new Sirene object on a different thread
    // and once it's ready (resources are loaded), swap it with the old one.
    // or just set an atomic boolean flag while the siren is set to nullptr
    // and it has been reloaded (can't afford to load all resources in each
    // plugin instance as memory cannot be shared in all DAWs)
    // => ACTUALLY THIS WILL BE DONE AT HIGHER LEVEL (INSTANTIATION AND SWAPPING)
    // TODO : create a dedicated class once we have a POC implemented in the processor
    // void setSirenId(sirenId id);

    void setSampleRate(double newSampleRate);
    void handleMidi(int status, int value1, int value2);
    void stopSiren();
#if COMPOSESIREN_RESETALLCONTROLLERS
    void resetSiren();
    void setControllerResetEnabled(bool enabled);
    void setChannelModePolicy(cs::DspChannelModePolicy policy);
#endif

    void beginProcessBlock();
    // this will compute the next sample to play and return it
    float process();

    // this will call Sirene::setnote() which makes the simulation move forward
    // this needs to be called at a fixed rate (maybe we could do it by
    // counting the audio samples -> yes, very probably !)
    void update();

    bool getIsNoteOn() const;
    float getCurrentPitch() const;
};

// SIREN VOICE WRAPPER CLASS ===================================================

class SirenVoice
{
    double lastSampleRate;
    // int lastBlockSize;

    std::atomic<SirenVoiceUnit*> currentSiren   { nullptr };
    std::atomic<SirenVoiceUnit*> discardedSiren { nullptr };
    std::atomic<bool> sirenIsLoading { false };

    SirenVoiceUnit* rawSiren;
#if COMPOSESIREN_RUST_DSP
    // Hot swap (COMPOSESIREN_RUST_DSP). A new unit (another siren, or the same siren with
    // another dsp.variant / dsp.dataset) is built off the audio thread and published in
    // pendingSiren (a siren change on the message thread, as before; a DSP change on a
    // loader thread). The audio thread adopts a siren change at the next block start, a
    // DSP change just before the next note-on that finds no note held, so the change never
    // cuts a held note, and a release only where the next note starts. It replays the
    // controllers and the pitch wheel into it, and fades the previous unit out over
    // fadeSeconds, then hands it to retiredSiren, which collectGarbage() frees on the
    // message thread. No allocation, lock or free on the audio thread.
    std::atomic<SirenVoiceUnit*> pendingSiren { nullptr };
    std::atomic<SirenVoiceUnit*> retiredSiren { nullptr };
    SirenVoiceUnit* fadingSiren = nullptr; // audio thread
    int fadeLeft = 0;                      // audio thread
    float fadeStep = 0.0f;                 // audio thread
    std::atomic<double> sampleRateForNew { 44100.0 };
    std::atomic<std::uint32_t> swaps { 0 };
    std::array<std::int8_t, 120> ccs {};   // audio thread: last value of each controller, -1 none
    int wheel1 = -1, wheel2 = -1;          // audio thread: last pitch wheel
    // guards the fields below and publishing (message and loader threads)
    std::mutex controlLock;
    std::condition_variable loaderWake;
    std::thread loader;
    bool loaderStop = false;
    bool loaderRequested = false;
    std::uint64_t generation = 0; // a publish of an older request is dropped
    std::string resourcesPathForNew;
    cs::DspChoice choiceForNew;
    std::atomic<bool> reloading { false };
    void publish(SirenVoiceUnit* unit, std::uint64_t forGeneration);
    void adoptPending(bool atNoteOn);
    void loaderLoop();
#endif
#if COMPOSESIREN_RESETALLCONTROLLERS
    // set from any thread, consumed by the audio thread in beginProcessBlock
    std::atomic<bool> resetRequested { false };
    // Request acceptance updates immediately; DSP interpretation uses a block snapshot.
    std::atomic<bool> resetRequestsEnabled { true };
    std::atomic<std::uint32_t> channelModePolicy { cs::DspChannelModePolicy{}.packed() };
#endif

protected:
    std::optional<sirenId> id { std::nullopt };

public:
    struct State {
        bool isNoteOn { false };
        float currentPitch { 0.0f };
        float level { 0.0f }; // output RMS of the last block (COMPOSESIREN_SIREN_WAVES)
    };

    //--------------------------------------------------------------------------
    // SIREN VOICE LISTENER ////////////////////////////////////////////////////
    // Listen to note on/off and pitch changes :
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void currentSirenId(sirenId) {}
        virtual void currentSirenState(sirenId, State) {}
    };
    //--------------------------------------------------------------------------

private:
    std::set<Listener*> listeners;

public:
    SirenVoice();
    virtual ~SirenVoice();

    // SirenVoiceUnit lifecycle management
    virtual void setSirenId(sirenId sid, const std::string& resourcesPath);
    bool getSirenId(sirenId& sid);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    void removeAllListeners();

    // use this at the start of the processBlock function
    // (returns true if currentSiren is not loading or nullptr),
    // then safely call handleMidi and process methods.
    bool isSirenLoading() const;
    bool getRawSirenHandle();
    // bool getRawSirenHandle(SirenVoiceUnit* target = nullptr);
    // use this at the end of the processBlock function.
    void deleteDiscarded();
#if COMPOSESIREN_RUST_DSP
    // message thread: rebuild this voice's siren with the current DSP choice on a
    // background thread; it plays from the next note-on with no note held
    void reloadDsp();
    // message thread (notifyListeners calls it): free the units done playing
    void collectGarbage();
    bool hasPendingSwap() const { return pendingSiren.load(std::memory_order_acquire) != nullptr; }
    bool isReloading() const { return reloading.load(std::memory_order_acquire); }
    std::uint32_t swapCount() const { return swaps.load(std::memory_order_acquire); }
    static constexpr double fadeSeconds = 0.02;
#endif

    // those are using getRawSirenHandle internally :
    virtual void setSampleRate(double newSampleRate);
    void stop();
#if COMPOSESIREN_RESETALLCONTROLLERS
    // Thread-safe: restores the existing controller map and requests zero
    // MIDI volume. Applied at the start of the next audio block.
    void requestReset();
    void setControllerResetEnabled(bool enabled) {
        setResetRequestsEnabled(enabled);
        if (enabled) channelModePolicy.fetch_or(1u, std::memory_order_release);
        else channelModePolicy.fetch_and(~1u, std::memory_order_release);
    }
    void setResetRequestsEnabled(bool enabled) {
        resetRequestsEnabled.store(enabled, std::memory_order_release);
    }
    void setChannelModePolicy(cs::DspChannelModePolicy policy) {
        channelModePolicy.store(policy.packed(), std::memory_order_release);
    }
#endif
    void update();

    // those are not using getRawSirenHandle
    // handle with care ! (see comments above)
    void handleMidi(int status, int value1, int value2);
    virtual void beginProcessBlock();
    float process();

    // the following method is thread safe and should be called
    // from a Timer callback to trigger notifications (calls
    // getIsNoteOn and getCurrentPitch under the hood)
    void notifyListeners();

protected:
    // output level reported to the listeners (SirenTrack measures it)
    virtual float getLevel() { return 0.0f; }

private:
    bool getIsNoteOn();
    float getCurrentPitch();
};

#endif //COMPOSESIREN_SIREN_VOICE_H
