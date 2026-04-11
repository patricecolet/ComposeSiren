//
// Created by joseph larralde on 07/02/2026.
//

#ifndef COMPOSESIREN_SIRENVOICE_H
#define COMPOSESIREN_SIRENVOICE_H

#include "../definitions/parameterDefinitions.h"
#include "../dsp/MidiIn.h"
#include "../dsp/Sirene.h"

// VOICE UNIT BASED ON FILE RESOURCES SET
// TO BE SWAPPED WITH ANOTHER INSTANCE

class SirenVoiceUnit {
    sirenId id;
    std::shared_ptr<const sirenData> data;
    std::unique_ptr<MidiIn> midiIn;
    std::unique_ptr<Sirene> siren;
    double sampleRate;
    int sampleCountForMidiInTimer;
    int setNoteCallbackPeriodSamples;
    int setNoteSampleCounter;

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

    // this will compute the next sample to play and return it
    float process();

    // this will call Sirene::setnote() which makes the simulation move forward
    // this needs to be called at a fixed rate (maybe we could do it by
    // counting the audio samples -> yes, very probably !)
    void update();
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

public:
    SirenVoice();
    ~SirenVoice();

    // SirenVoiceUnit lifecycle management
    void setSirenId(
        sirenId id,
        const std::string& resourcesPath
    );

    // use this at the start of the processBlock function
    // (returns true if currentSiren is not loading or nullptr),
    // then safely call handleMidi and process methods.
    bool isSirenLoading() const;
    bool getRawSirenHandle(SirenVoiceUnit* target = nullptr);
    // use this at the end of the processBlock function.
    void deleteDiscarded();

    // those are using getRawSirenHandle internally :
    void setSampleRate(double newSampleRate);
    void stop();
    void update();

    // those are not using getRawSirenHandle
    // handle with care ! (see comments above)
    void handleMidi(int status, int value1, int value2);
    float process();
};
//     sirenId id;
// 	std::shared_ptr<const sirenData> data;
//     std::unique_ptr<MidiIn> midiIn;
//     std::unique_ptr<Sirene> siren;
//     double sampleRate;
//     int sampleCountForMidiInTimer;
//     int setNoteCallbackPeriodSamples;
//     int setNoteSampleCounter;
//
// public:
//     SirenVoice(sirenId id, const std::string& resourcesPath);
//     ~SirenVoice() = default;
//
// 	// this method should check if siren is nullptr
// 	// if it's not, instantiate a new Sirene object on a different thread
// 	// and once it's ready (resources are loaded), swap it with the old one.
// 	// or just set an atomic boolean flag while the siren is set to nullptr
// 	// and it has been reloaded (can't afford to load all resources in each
// 	// plugin instance as memory cannot be shared in all DAWs)
//     // => ACTUALLY THIS WILL BE DONE AT HIGHER LEVEL (INSTANTIATION AND SWAPPING)
//     // TODO : create a dedicated class once we have a POC implemented in the processor
// 	// void setSirenId(sirenId id);
//
//     void setSampleRate(double newSampleRate);
//
//     void handleMidi(int status, int value1, int value2);
//
//     void stopSiren();
//
//     // this will compute the next sample to play and return it
// 	float process();
//
// 	// this will call Sirene::setnote() which makes the simulation move forward
// 	// this needs to be called at a fixed rate (maybe we could do it by
// 	// counting the audio samples -> yes, very probably !)
// 	void update();
// };

#endif //COMPOSESIREN_SIRENVOICE_H
