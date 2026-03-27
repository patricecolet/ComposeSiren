//
// Created by joseph larralde on 07/02/2026.
//

#include "SirenVoice.h"

SirenVoice::SirenVoice(sirenId sid, const std::string& resourcesPath) :
    id(sid),
    sampleCountForMidiInTimer(0),
    setNoteSampleCounter(0)
{
    data = sirenPropertiesById.at(id);
    siren = std::make_unique<Sirene>(id, resourcesPath);
    // attempt to get rid of aliasing-like sounds in last note of soprano/piccolo
    // siren->changeQualite(20);

    auto onVolumeChanged = [this](float v) { siren->setVelocite(v);    };
    auto onNoteChanged   = [this](float v) { siren->setnoteFromExt(v); };

    midiIn = std::make_unique<MidiIn>(id, onVolumeChanged, onNoteChanged);
    // midiIn->resetSirene();
    // setSampleRate(44100.0);
}

//==============================================================================
// From CS_MidiIn :

void SirenVoice::setSampleRate(double newSampleRate) {
    sampleRate = newSampleRate;
    // original 1kHz juce::Timer frequency for siren->setnote() callback
    float updateFrequency = 1000.0f;
    setNoteCallbackPeriodSamples = static_cast<int>(sampleRate / updateFrequency);
    midiIn->setSampleRate(sampleRate);
    siren->setSampleRate(sampleRate);
}

void SirenVoice::handleMidi(int status, int value1, int value2) {
    // ignore channel info.
    // detect note on, note off, cc and pitchwheel events.

    // sanitize input (really necessary ?):
    if (status > 0xFF || value1 > 0x7F || value2 > 0x7F) { return; }

    // first nibble goes from 8 to F (ms bit is always on)
    // second nibble is the channel so we ignore it
    if (status >> 4 == 0x8) { // note off
        midiIn->realTimeStopNote(value1);
    } else if (status >> 4 == 0x9) { // note on
        midiIn->realTimeStartNote(value1, value2);
    } else if (status >> 4 == 0xB) { // cc
        midiIn->handleControlChange(value1, value2);
    } else if (status >> 4 & 0xE) { // pitch bend
        midiIn->handlePitchWheel(value1, value2);
    }
}

void SirenVoice::stopSiren() {
    midiIn->stopSirene();
}

float SirenVoice::process() {
    // in original code, this timer updating dsp computations related to note slide, vibrato & tremolo is
    // only called once every block, and block is hardcoded to be 512
    // implement this with a counter
    // todo : try varying the block size and listen.
    //  does it really make any difference ? could it be called sample-wise ?
    //  -> answer : both counters just modify the inertia of the rotor
    //  (ie : scale portamento time by a certain amount)
    //  these don't seem very critical, so let's keep them as they were.
    if (sampleCountForMidiInTimer % 512 == 0) {
        midiIn->timerAudio();
        sampleCountForMidiInTimer = 0;
    }
    ++sampleCountForMidiInTimer;

    if (setNoteSampleCounter % setNoteCallbackPeriodSamples == 0) {
        siren->setnote();
        setNoteSampleCounter = 0;
    }
    ++setNoteSampleCounter;

    // compute the next sample to play
    return siren->calculwave();
}

// this will call Sirene::setnote() which makes the simulation move forward
// this needs to be called at a fixed rate (maybe we could do it by
// counting the audio samples -> yes, very probably !)
// TODO : a timerCallback calls setnote on each siren at a (arbitrary ?) 1kHz frequency
//  is this frequency sensitive ?
//  see the 512 samples period above for the execution of midiIn->timerAudio()
//  are those kind of related -> try other values and listen to the difference
void SirenVoice::update() {
    siren->setnote();
}
