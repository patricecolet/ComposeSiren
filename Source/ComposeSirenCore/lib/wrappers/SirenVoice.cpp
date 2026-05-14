//
// Created by joseph larralde on 07/02/2026.
//

#include <ranges>
#include "SirenVoice.h"

SirenVoiceUnit::SirenVoiceUnit(sirenId sid, const std::string& resourcesPath) :
    id(sid),
    sampleCountForMidiInTimer(0),
    setNoteSampleCounter(0)
{
    data = sirenPropertiesById.at(id);
    const auto& maxRange = data->velocityRanges.rbegin()->second;

    // we really need to settle on what is C0 and C1 !
    // this +12 -12 fix is scattered through the code
    // (see DbRangesMidiKeyboardComponent)
    // is the legacy DSP code the source of truth ? this means JUCE is WRONG ?
    maxNote = get<1>(maxRange) + 12;

    siren = std::make_unique<Sirene>(id, resourcesPath);
    // attempt to get rid of aliasing-like sounds in last note of soprano/piccolo
    // siren->changeQualite(20);

    auto onVolumeChanged = [this](float v) { siren->setVelocite(v);    };
    auto onNoteChanged   = [this](float v) { siren->setnoteFromExt(v); };

    midiIn = std::make_unique<MidiIn>(id, onVolumeChanged, onNoteChanged);
}

//==============================================================================
// From CS_MidiIn :

void SirenVoiceUnit::setSampleRate(double newSampleRate) {
    sampleRate = newSampleRate;
    // original 1kHz juce::Timer frequency for siren->setnote() callback
    float updateFrequency = 1000.0f;
    setNoteCallbackPeriodSamples = static_cast<int>(sampleRate / updateFrequency);
    midiIn->setSampleRate(sampleRate);
    siren->setSampleRate(sampleRate);
}

void SirenVoiceUnit::handleMidi(int status, int value1, int value2) {
    // ignore channel info.
    // detect note on, note off, cc and pitchwheel events.

    // sanitize input (really necessary ?):
    if (status > 0xFF || value1 > 0x7F || value2 > 0x7F) { return; }

    // first nibble goes from 8 to F (ms bit is always on)
    // second nibble is the channel so we ignore it
    if (status >> 4 == 0x8) { // note off
        value1 = value1 < maxNote ? value1 : maxNote;
        midiIn->realTimeStopNote(value1);
        ino = false;
    } else if (status >> 4 == 0x9) { // note on
        value1 = value1 < maxNote ? value1 : maxNote;
        midiIn->realTimeStartNote(value1, value2);
        ino = true;
    } else if (status >> 4 == 0xB) { // cc
        midiIn->handleControlChange(value1, value2);
    } else if (status >> 4 & 0xE) { // pitch bend
        midiIn->handlePitchWheel(value1, value2);
    }
}

void SirenVoiceUnit::stopSiren() {
    midiIn->stopSirene();
}

void SirenVoiceUnit::beginProcessBlock()
{
    isNoteOn.store(ino, std::memory_order_relaxed);
    currentPitch.store(cp, std::memory_order_relaxed);
}

float SirenVoiceUnit::process() {
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
        cp = siren->getCurrentPitch() * 0.01f; // pitch is in cents
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
void SirenVoiceUnit::update() {
    siren->setnote();
}

bool SirenVoiceUnit::getIsNoteOn() const {
    return isNoteOn.load(std::memory_order_relaxed);
}

float SirenVoiceUnit::getCurrentPitch() const {
    return currentPitch.load(std::memory_order_relaxed);
}

// SIREN VOICE WRAPPER =========================================================

SirenVoice::SirenVoice() :
    lastSampleRate(44100.0), rawSiren(nullptr)
{}

SirenVoice::~SirenVoice()
{
    delete currentSiren.load();
    delete discardedSiren.load();
    removeAllListeners();
}

void SirenVoice::addListener(Listener* listener)
{
    listeners.insert(listener);
}

void SirenVoice::removeListener(Listener* listener)
{
    if (listeners.find(listener) != listeners.end()) {
        listeners.erase(listener);
    }
}

void SirenVoice::removeAllListeners()
{
    listeners.clear();
}

void SirenVoice::setSirenId(
    sirenId sid,
    const std::string& resourcesPath
) {
    sirenIsLoading.store(true, std::memory_order_release);
    id = sid;
    auto* newSiren = new SirenVoiceUnit(sid, resourcesPath);
    newSiren->setSampleRate(lastSampleRate);
    auto* oldSiren = currentSiren.exchange(newSiren, std::memory_order_acq_rel);
    // safe to delete the old Siren at the end of processBlock, the audio thread
    // might still be using it right now (think in terms of ownership horizon)
    // -> call deleteDiscarded at the end of processBlock !
    discardedSiren.store(oldSiren, std::memory_order_release);
    sirenIsLoading.store(false, std::memory_order_release);
    for (const auto& l : listeners) { l->currentSirenId(id.value()); }
}

bool SirenVoice::getSirenId(sirenId& sid)
{
    if (id.has_value()) {
        sid = id.value();
        return true;
    }
    return false;
}

bool SirenVoice::isSirenLoading() const
{
    return sirenIsLoading.load(std::memory_order_acquire);
}

bool SirenVoice::getRawSirenHandle()
{
    rawSiren = currentSiren.load(std::memory_order_acquire);
    if (rawSiren == nullptr) { return false; }
    return true;
}

// bool SirenVoice::getRawSirenHandle(SirenVoiceUnit* target)
// {
//     rawSiren = currentSiren.load(std::memory_order_acquire);
//     target = rawSiren;
//     if (rawSiren == nullptr) { return false; }
//     return true;
// }

void SirenVoice::deleteDiscarded()
{
    delete discardedSiren.exchange(nullptr, std::memory_order_release);
}

void SirenVoice::setSampleRate(double sampleRate)
{
    lastSampleRate = sampleRate;
    if (getRawSirenHandle()) { rawSiren->setSampleRate(sampleRate); }
}

void SirenVoice::stop()
{
    if (getRawSirenHandle()) { rawSiren->stopSiren(); }
}

void SirenVoice::update()
{
    if (getRawSirenHandle()) { rawSiren->update(); }
}

void SirenVoice::handleMidi(int status, int value1, int value2)
{
    rawSiren->handleMidi(status, value1, value2);
}

void SirenVoice::beginProcessBlock()
{
    rawSiren->beginProcessBlock();
}

float SirenVoice::process()
{
    return rawSiren->process();
}

bool SirenVoice::getIsNoteOn()
{
    if (getRawSirenHandle()) { return rawSiren->getIsNoteOn(); }
    return false;
}

float SirenVoice::getCurrentPitch()
{
    if (getRawSirenHandle()) { return rawSiren->getCurrentPitch(); }
    return 0.0;
}

void SirenVoice::notifyListeners()
{
    if (!id.has_value()) { return; }

    for (const auto& l : listeners) {
        l->currentSirenState(id.value(), {
            getIsNoteOn(),
            getCurrentPitch()
        });
    }
}


