//
// Created by joseph larralde on 17/02/2026.
//

#include "SirenEnsemble.h"

#include <ranges>

//------------------------------------------------------------------------------
// SIREN TRACK /////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------

SirenTrack::SirenTrack(sirenId id, const std::string& resourcesPath)
{
    SirenVoice::setSirenId(id, resourcesPath);
    computePanningGains();
}

// SirenTrack::~SirenTrack() {}

void SirenTrack::setSampleRate(double sr) {
    SirenVoice::setSampleRate(sr);
    gr.setSampleRate(sr);
    pr.setSampleRate(sr);
}
void SirenTrack::setPanning(float p) { panning.store(p,std::memory_order_relaxed); }
void SirenTrack::setOutputGain(float g) { gain.store(g, std::memory_order_relaxed); }

// call this at the beginning of the audio callback
void SirenTrack::beginProcessBlock() {
    SirenVoice::beginProcessBlock();
    gr.setTarget(gain.load(std::memory_order_relaxed));
    pr.setTarget(panning.load(std::memory_order_relaxed));
}

// call this for each sample during the audio callback
void SirenTrack::process(float *l, float* r) {
    gr.process(g);
    // only recompute panning gains if needed (i.e if we are ramping) :
    if (pr.process(p)) { computePanningGains(); }
    float s = SirenVoice::process() * g;
    *l = s * lp;
    *r = s * rp;
}

// we can add sqrt panning law and others later if we like
void SirenTrack::computePanningGains() {
    p = (p + 1.0f) * 0.5f; // [-1;1] to [0;1]
    lp = std::cos(p * pi_2);
    rp = std::cos((1.0f - p) * pi_2);
}

//------------------------------------------------------------------------------
// SIREN ENSEMBLE //////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------

SirenEnsemble::SirenEnsemble(const std::vector<sirenId>& ids,
                             const std::string& resourcesPath) {
    for (auto id : ids) {
        tracks[id] = std::make_unique<SirenTrack>(id, resourcesPath);
    }
}

bool SirenEnsemble::areSirensLoading() const {
    for (auto& t : tracks | std::views::values) {
        if (t->isSirenLoading()) { return true; }
    }
    return false;
}

bool SirenEnsemble::getRawSirenHandles() const {
    for (auto& t : tracks | std::views::values) {
        if (!t->getRawSirenHandle()) { return false; }
    }
    return true;
}

std::vector<sirenId> SirenEnsemble::getSirenIds() const {
    auto keys = std::views::keys(tracks);
    std::vector<sirenId> ids{keys.begin(), keys.end()};
    return ids;
}

void SirenEnsemble::addListener(SirenVoice::Listener* listener) {
    for (auto& t : tracks | std::views::values) { t->addListener(listener); }
}

void SirenEnsemble::removeListener(SirenVoice::Listener* listener) {
    for (auto& t : tracks | std::views::values) { t->removeListener(listener); }
}

void SirenEnsemble::removeAllListeners() {
    for (auto& t : tracks | std::views::values) { t->removeAllListeners(); }
}

void SirenEnsemble::notifyListeners() {
    for (auto& t : tracks | std::views::values) { t->notifyListeners(); }
}

void SirenEnsemble::setSampleRate(double sr) {
    for (auto& t : tracks | std::views::values) { t->setSampleRate(sr); }
    vr.setSampleRate(sr);
}

void SirenEnsemble::update() {
    for (auto& t : tracks | std::views::values) { t->update(); }
}

void SirenEnsemble::stop(std::optional<sirenId> id) {
    if (id.has_value()) {
        tracks.at(id.value())->stop();
    } else {
        for (auto& t : tracks | std::views::values) { t->stop(); }
    }
}

void SirenEnsemble::setPanning(sirenId id, float p) {
    tracks.at(id)->setPanning(p);
}

void SirenEnsemble::setOutputGain(sirenId id, float g) {
    tracks.at(id)->setOutputGain(g);
}

void SirenEnsemble::setMasterVolume(float v) {
    masterVolume.store(v, std::memory_order_relaxed);
}

void SirenEnsemble::handleMidi(int status, int value1, int value2) {
    // high nibble goes from 8 to F (ms bit is always on)
    // low nibble is the channel so we just check it and forward
    // the message to the appropriate siren
    if (status > 0xFF) { return; } // sanitize
    OneBasedMidiChannel channel = {.oneBased = (status & 0x0F) + 1};

    auto it = sirenPropertiesByChannel.find(channel);
    if (it != sirenPropertiesByChannel.end()) {
        auto id = it->second->id;
        tracks.at(id)->handleMidi(status, value1, value2);
    }
}

void SirenEnsemble::beginProcessBlock() {
    for (auto& t : tracks | std::views::values) { t->beginProcessBlock(); }
    vr.setTarget(masterVolume.load(std::memory_order_relaxed));
}

void SirenEnsemble::process(float* l, float* r) {
    *l = *r = 0;
    float tmpl = 0;
    float tmpr = 0;
    for (auto& t : tracks) {
        t.second->process(&tmpl, &tmpr);
        *l += tmpl;
        *r += tmpr;
    }
    vr.process(v);
    *l *= v;
    *r *= v;
    // *l /= tracks.size();
    // *r /= tracks.size();
}

void SirenEnsemble::deleteDiscarded() const {
    for (auto& t : tracks | std::views::values) { t->deleteDiscarded(); }
}
