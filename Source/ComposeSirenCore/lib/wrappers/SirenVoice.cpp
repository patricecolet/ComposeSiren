//
// Created by joseph larralde on 07/02/2026.
//

#include <ranges>
#include "SirenVoice.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

#if COMPOSESIREN_RUST_DSP
SirenVoiceUnit::SirenVoiceUnit(sirenId sid, const std::string& resourcesPath, const cs::DspChoice& choice) :
#else
SirenVoiceUnit::SirenVoiceUnit(sirenId sid, const std::string& resourcesPath) :
#endif
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

#if COMPOSESIREN_RUST_DSP
    if (!choice.isCpp()) {
        const std::string name = "S" + std::to_string(static_cast<int>(id) + 1);
        rustSiren.reset(csr_siren_new(resourcesPath.c_str(), name.c_str(), choice.variant.c_str(), choice.dataset.c_str()));
        if (rustSiren) csr_siren_set_skip_silent(rustSiren.get(), choice.skipSilent);
        if (!rustSiren) {
            char err[512];
            sdsp_last_error(reinterpret_cast<uint8_t*>(err), sizeof err);
            std::fprintf(stderr, "ComposeSiren: Rust DSP %s/%s for %s failed (%s), playing the C++ one\n",
                         choice.variant.c_str(), choice.dataset.c_str(), name.c_str(), err);
        }
    }
    if (rustSiren) {
        // the C++ MidiIn passes floats on to Sirene's int setters: the same conversion
        auto onVolumeChanged = [this](float v) { csr_siren_set_velocite(rustSiren.get(), static_cast<int>(v)); };
        auto onNoteChanged   = [this](float v) { csr_siren_set_note_from_ext(rustSiren.get(), static_cast<int>(v)); };
        midiIn = std::make_unique<MidiIn>(id, onVolumeChanged, onNoteChanged);
        return;
    }
#endif
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
#if COMPOSESIREN_RUST_DSP
    if (rustSiren) { csr_siren_set_sample_rate(rustSiren.get(), sampleRate); return; }
#endif
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
#if COMPOSESIREN_RESETALLCONTROLLERS
        if (midiIn->isControllerResetEnabled()
            && midiIn->acceptsChannelMode(value1, value2)) {
            ino = false;
        }
#endif
    } else if (status >> 4 & 0xE) { // pitch bend
        midiIn->handlePitchWheel(value1, value2);
    }
}

#if COMPOSESIREN_RUST_DSP
namespace {
// the C++ Sirene::Handoff and the C ABI's CsrHandoff hold the same fields
void toCsr(const Sirene::Handoff& a, CsrHandoff& b) {
    b.note_voulue_avant_slide = a.noteVoulueAvantSlide;
    b.note_encour = a.noteEncour;
    b.inter_depart = a.interDepart;
    b.ampvoulu = a.ampvoulu;
    b.ampvouluz = a.ampvouluz;
    b.count_k_inf = a.countKInf;
    b.count_k_sup = a.countKSup;
    b.count_p_inf = a.countPInf;
    b.count_p_sup = a.countPSup;
    std::copy(std::begin(a.ampz), std::end(a.ampz), std::begin(b.ampz));
    std::copy(std::begin(a.amp), std::end(a.amp), std::begin(b.amp));
    std::copy(std::begin(a.phaseInf), std::end(a.phaseInf), std::begin(b.phase_inf));
    std::copy(std::begin(a.phaseSup), std::end(a.phaseSup), std::begin(b.phase_sup));
    b.ancienne_wave_inf = a.anciennewaveInf;
    b.trio_phi = a.trioPhi;
}
void fromCsr(const CsrHandoff& b, Sirene::Handoff& a) {
    a.noteVoulueAvantSlide = b.note_voulue_avant_slide;
    a.noteEncour = b.note_encour;
    a.interDepart = b.inter_depart;
    a.ampvoulu = b.ampvoulu;
    a.ampvouluz = b.ampvouluz;
    a.countKInf = b.count_k_inf;
    a.countKSup = b.count_k_sup;
    a.countPInf = b.count_p_inf;
    a.countPSup = b.count_p_sup;
    std::copy(std::begin(b.ampz), std::end(b.ampz), std::begin(a.ampz));
    std::copy(std::begin(b.amp), std::end(b.amp), std::begin(a.amp));
    std::copy(std::begin(b.phase_inf), std::end(b.phase_inf), std::begin(a.phaseInf));
    std::copy(std::begin(b.phase_sup), std::end(b.phase_sup), std::begin(a.phaseSup));
    a.anciennewaveInf = b.ancienne_wave_inf;
    a.trioPhi = b.trio_phi;
    a.trio = b.trio;
}
}

bool SirenVoiceUnit::takeOver(const SirenVoiceUnit& o) {
    midiIn->copyStateFrom(*o.midiIn);
    sampleCountForMidiInTimer = o.sampleCountForMidiInTimer;
    setNoteSampleCounter = o.setNoteSampleCounter;
    ino = o.ino;
    cp = o.cp;
    // on the audio thread: no allocation (both structs on the stack, ~5 KB)
    Sirene::Handoff h;
    CsrHandoff c;
    if (o.rustSiren) { csr_siren_get_handoff(o.rustSiren.get(), &c); fromCsr(c, h); }
    else h = o.siren->getHandoff();
    const int wasTrio = h.trio;
    if (rustSiren) { toCsr(h, c); csr_siren_set_handoff(rustSiren.get(), &c); }
    else siren->setHandoff(h);
    int isTrio = 0;
    if (rustSiren) { csr_siren_get_handoff(rustSiren.get(), &c); isTrio = c.trio; }
    return wasTrio == isTrio;
}
#endif

void SirenVoiceUnit::stopSiren() {
    midiIn->stopSirene();
}

#if COMPOSESIREN_RESETALLCONTROLLERS
void SirenVoiceUnit::setControllerResetEnabled(bool enabled) {
    midiIn->setControllerResetEnabled(enabled);
}

void SirenVoiceUnit::setChannelModePolicy(cs::DspChannelModePolicy policy) {
    midiIn->setChannelModePolicy(policy);
}

void SirenVoiceUnit::resetSiren() {
    midiIn->resetSirene();
    ino = false;
}

#endif
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

#if COMPOSESIREN_RUST_DSP
    if (rustSiren) {
        if (setNoteSampleCounter % setNoteCallbackPeriodSamples == 0) {
            csr_siren_setnote(rustSiren.get());
            cp = csr_siren_current_pitch(rustSiren.get()) * 0.01f; // pitch is in cents
            setNoteSampleCounter = 0;
        }
        ++setNoteSampleCounter;
        return csr_siren_calculwave(rustSiren.get());
    }
#endif
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
#if COMPOSESIREN_RUST_DSP
    if (rustSiren) { csr_siren_setnote(rustSiren.get()); return; }
#endif
    siren->setnote();
}

bool SirenVoiceUnit::getIsNoteOn() const {
    return isNoteOn.load(std::memory_order_relaxed);
}

float SirenVoiceUnit::getCurrentPitch() const {
    return currentPitch.load(std::memory_order_relaxed);
}

// SIREN VOICE WRAPPER =========================================================

#if !COMPOSESIREN_RUST_DSP

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

#if COMPOSESIREN_RESETALLCONTROLLERS
void SirenVoice::requestReset()
{
    if (resetRequestsEnabled.load(std::memory_order_acquire))
        resetRequested.store(true, std::memory_order_release);
}

#endif
void SirenVoice::update()
{
    if (getRawSirenHandle()) { rawSiren->update(); }
}

void SirenVoice::handleMidi(int status, int value1, int value2)
{
    rawSiren->handleMidi(status, value1, value2);
}

void SirenVoice::handleMidiWithoutHandle(int status, int value1, int value2)
{
    const int kind = status >> 4;
    if (kind == 0x8 || kind == 0x9) return;
    if (auto* cur = currentSiren.load(std::memory_order_acquire)) cur->handleMidi(status, value1, value2);
}

void SirenVoice::beginProcessBlock()
{
#if COMPOSESIREN_RESETALLCONTROLLERS
    const auto policy = cs::DspChannelModePolicy::unpack(channelModePolicy.load(std::memory_order_acquire));
    rawSiren->setChannelModePolicy(policy);
    // Consume pending requests even when disabled; don't replay them later.
    if (resetRequested.exchange(false, std::memory_order_acq_rel) && policy.enabled) {
        rawSiren->resetSiren();
    }
#endif
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
            getCurrentPitch(),
            getLevel()
        });
    }
}
#else // COMPOSESIREN_RUST_DSP: the hot-swapping voice (SirenVoice.h)

SirenVoice::SirenVoice() :
    lastSampleRate(44100.0), rawSiren(nullptr)
{
    ccs.fill(-1);
}

SirenVoice::~SirenVoice()
{
    {
        std::lock_guard g(controlLock);
        loaderStop = true;
    }
    loaderWake.notify_all();
    if (loader.joinable()) loader.join();
    delete currentSiren.load();
    delete pendingSiren.load();
    delete retiredSiren.load();
    delete fadingSiren;
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

// message or loader thread, controlLock held: hand `unit` to the audio thread, unless a
// newer request came meanwhile; a unit published but not yet adopted is still ours
void SirenVoice::publish(SirenVoiceUnit* unit, std::uint64_t forGeneration)
{
    if (forGeneration != generation) { delete unit; return; }
    if (auto* old = pendingSiren.exchange(unit, std::memory_order_acq_rel)) {
        unit->adoptAtOnce = unit->adoptAtOnce || old->adoptAtOnce; // a siren change still pending
        delete old;
    }
}

void SirenVoice::setSirenId(
    sirenId sid,
    const std::string& resourcesPath
) {
    sirenIsLoading.store(true, std::memory_order_release);
    // built here, as before; the DSP choice is the current one (DspChoice.h)
    auto* newSiren = new SirenVoiceUnit(sid, resourcesPath);
    newSiren->setSampleRate(sampleRateForNew.load(std::memory_order_acquire));
    newSiren->adoptAtOnce = true;
    {
        std::lock_guard g(controlLock);
        id = sid;
        resourcesPathForNew = resourcesPath;
        publish(newSiren, ++generation);
    }
    sirenIsLoading.store(false, std::memory_order_release);
    for (const auto& l : listeners) { l->currentSirenId(id.value()); }
}

void SirenVoice::reloadDsp()
{
    if (!id.has_value()) return;
    {
        std::lock_guard g(controlLock);
        if (resourcesPathForNew.empty()) return;
        choiceForNew = cs::currentDspChoice();
        ++generation;
        loaderRequested = true;
        reloading.store(true, std::memory_order_release);
        if (!loader.joinable()) loader = std::thread([this] { loaderLoop(); });
    }
    loaderWake.notify_all();
}

// the loader thread: builds the unit of the latest request, off the audio thread
void SirenVoice::loaderLoop()
{
    std::unique_lock g(controlLock);
    for (;;) {
        loaderWake.wait(g, [this] { return loaderStop || loaderRequested; });
        if (loaderStop) return;
        loaderRequested = false;
        const auto gen = generation;
        const auto sid = id.value();
        const auto path = resourcesPathForNew;
        const auto choice = choiceForNew;
        g.unlock();
        auto* unit = new SirenVoiceUnit(sid, path, choice);
        unit->setSampleRate(sampleRateForNew.load(std::memory_order_acquire));
        g.lock();
        publish(unit, gen);
        if (!loaderRequested) reloading.store(false, std::memory_order_release);
    }
}

// audio thread: take the pending unit's place when allowed (SirenVoice.h)
void SirenVoice::adoptPending(bool atNoteOn)
{
    auto* p = pendingSiren.load(std::memory_order_acquire);
    if (p == nullptr) return;
    auto* cur = currentSiren.load(std::memory_order_relaxed);
    if (cur != nullptr) {
        if (fadingSiren != nullptr) return;           // the previous fade isn't over
        // a DSP change with a note held takes the note over at once (takeOver: the new unit
        // carries on from the old one's state, under the crossfade); with none held it waits
        // for the next note-on, so the previous unit keeps its release until a new note
        // would cut it anyway
        if (!p->adoptAtOnce && !atNoteOn && !cur->isHeld()) return;
    }
    p = pendingSiren.exchange(nullptr, std::memory_order_acq_rel);
    if (p == nullptr) return;
    fadeInNew = cur != nullptr && !p->adoptAtOnce && cur->isHeld();
    if (fadeInNew) {
        fadeEqualPower = !p->takeOver(*cur); // the controllers and the wheel come with the MidiIn state
    } else {
        for (int c = 0; c < static_cast<int>(ccs.size()); ++c)
            if (ccs[static_cast<size_t>(c)] >= 0) p->handleMidi(0xB0, c, ccs[static_cast<size_t>(c)]);
        if (wheel1 >= 0) p->handleMidi(0xE0, wheel1, wheel2);
    }
    currentSiren.store(p, std::memory_order_release);
    rawSiren = p;
#if COMPOSESIREN_RESETALLCONTROLLERS
    p->setChannelModePolicy(cs::DspChannelModePolicy::unpack(channelModePolicy.load(std::memory_order_acquire)));
#endif
    if (cur != nullptr) {
        fadingSiren = cur;
        fadeLeft = std::max(1, static_cast<int>(fadeSeconds * lastSampleRate));
        fadeStep = 1.0f / static_cast<float>(fadeLeft);
    }
    swaps.fetch_add(1, std::memory_order_acq_rel);
}

#if COMPOSESIREN_RESETALLCONTROLLERS
void SirenVoice::requestReset()
{
    if (resetRequestsEnabled.load(std::memory_order_acquire))
        resetRequested.store(true, std::memory_order_release);
}
#endif

void SirenVoice::collectGarbage()
{
    delete retiredSiren.exchange(nullptr, std::memory_order_acq_rel);
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

// audio thread, at the start of a block
bool SirenVoice::getRawSirenHandle()
{
    adoptPending(false);
    rawSiren = currentSiren.load(std::memory_order_acquire);
    return rawSiren != nullptr;
}

// units are freed by collectGarbage, off the audio thread
void SirenVoice::deleteDiscarded() {}

// not while the audio thread runs (prepareToPlay)
void SirenVoice::setSampleRate(double sampleRate)
{
    lastSampleRate = sampleRate;
    sampleRateForNew.store(sampleRate, std::memory_order_release);
    if (auto* cur = currentSiren.load(std::memory_order_acquire)) cur->setSampleRate(sampleRate);
    if (fadingSiren != nullptr) fadingSiren->setSampleRate(sampleRate);
    std::lock_guard g(controlLock);
    if (auto* p = pendingSiren.load(std::memory_order_acquire)) p->setSampleRate(sampleRate);
}

void SirenVoice::stop()
{
    if (auto* cur = currentSiren.load(std::memory_order_acquire)) cur->stopSiren();
}

void SirenVoice::update()
{
    if (auto* cur = currentSiren.load(std::memory_order_acquire)) cur->update();
}

void SirenVoice::handleMidiWithoutHandle(int status, int value1, int value2)
{
    const int kind = status >> 4;
    if (kind == 0x8 || kind == 0x9) return;
    if (kind == 0xB && value1 >= 0 && value1 < static_cast<int>(ccs.size())) {
        ccs[static_cast<size_t>(value1)] = static_cast<std::int8_t>(value2 & 0x7F);
    } else if (kind == 0xB && value1 == 121) {
        ccs.fill(-1);
        wheel1 = wheel2 = -1;
    } else if (kind == 0xE) {
        wheel1 = value1 & 0x7F;
        wheel2 = value2 & 0x7F;
    }
    if (auto* cur = currentSiren.load(std::memory_order_acquire)) cur->handleMidi(status, value1, value2);
}

void SirenVoice::handleMidi(int status, int value1, int value2)
{
    const int kind = status >> 4;
    if (kind == 0x9 && value2 > 0) {
        adoptPending(true);
    } else if (kind == 0xB && value1 >= 0 && value1 < static_cast<int>(ccs.size())) {
        ccs[static_cast<size_t>(value1)] = static_cast<std::int8_t>(value2 & 0x7F);
    } else if (kind == 0xB && value1 == 121) { // reset all controllers
        ccs.fill(-1);
        wheel1 = wheel2 = -1;
    } else if (kind == 0xE) {
        wheel1 = value1 & 0x7F;
        wheel2 = value2 & 0x7F;
    }
    rawSiren->handleMidi(status, value1, value2);
}

void SirenVoice::beginProcessBlock()
{
    if (fadingSiren != nullptr && fadeLeft == 0) {
        SirenVoiceUnit* none = nullptr;
        if (retiredSiren.compare_exchange_strong(none, fadingSiren, std::memory_order_acq_rel))
            fadingSiren = nullptr;
    }
#if COMPOSESIREN_RESETALLCONTROLLERS
    const auto policy = cs::DspChannelModePolicy::unpack(channelModePolicy.load(std::memory_order_acquire));
    rawSiren->setChannelModePolicy(policy);
    if (resetRequested.exchange(false, std::memory_order_acq_rel) && policy.enabled) {
        rawSiren->resetSiren();
    }
#endif
    rawSiren->beginProcessBlock();
}

float SirenVoice::process()
{
    float s = rawSiren->process();
    if (fadeLeft > 0) {
        const float g = static_cast<float>(fadeLeft) * fadeStep;
        if (fadeInNew && fadeEqualPower) {
            // two unrelated waves: their powers add, so cos/sin keep the level
            const float a = 1.5707963f * g;
            s = s * std::cos(a) + fadingSiren->process() * std::sin(a);
        } else {
            if (fadeInNew) s *= 1.0f - g;
            s += fadingSiren->process() * g;
        }
        --fadeLeft;
    }
    return s;
}

bool SirenVoice::getIsNoteOn()
{
    if (auto* cur = currentSiren.load(std::memory_order_acquire)) return cur->getIsNoteOn();
    return false;
}

float SirenVoice::getCurrentPitch()
{
    if (auto* cur = currentSiren.load(std::memory_order_acquire)) return cur->getCurrentPitch();
    return 0.0;
}

void SirenVoice::notifyListeners()
{
    collectGarbage();
    if (!id.has_value()) { return; }

    for (const auto& l : listeners) {
        l->currentSirenState(id.value(), {
            getIsNoteOn(),
            getCurrentPitch(),
            getLevel()
        });
    }
}

#endif // COMPOSESIREN_RUST_DSP
