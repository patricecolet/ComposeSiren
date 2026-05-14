//
// Created by joseph larralde on 07/02/2026.
//

#ifndef COMPOSESIREN_SIREN_VOICE_H
#define COMPOSESIREN_SIREN_VOICE_H

#include <set>
#include "../definitions/parameterDefinitions.h"
#include "../dsp/MidiIn.h"
#include "../dsp/Sirene.h"

// VOICE UNIT BASED ON FILE RESOURCES SET
// TO BE SWAPPED WITH ANOTHER INSTANCE

class SirenVoiceUnit {
    sirenId id;
    std::shared_ptr<const sirenData> data;
    int maxNote; // as defined in velocityRanges
    std::unique_ptr<MidiIn> midiIn;
    std::unique_ptr<Sirene> siren;
    double sampleRate;
    int sampleCountForMidiInTimer;
    int setNoteCallbackPeriodSamples;
    int setNoteSampleCounter;

    std::atomic<bool> isNoteOn { false };
    bool ino = false;
    std::atomic<float> currentPitch { 0.0f };
    float cp = 0.0f;

public:
    SirenVoiceUnit(sirenId id, const std::string& resourcesPath);
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

protected:
    std::optional<sirenId> id { std::nullopt };

public:
    struct State {
        bool isNoteOn { false };
        float currentPitch { 0.0f };
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

    // those are using getRawSirenHandle internally :
    virtual void setSampleRate(double newSampleRate);
    void stop();
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

private:
    bool getIsNoteOn();
    float getCurrentPitch();
};

#endif //COMPOSESIREN_SIREN_VOICE_H
