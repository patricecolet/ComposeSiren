// The reference for siren-dsp's tests: the plugin's own DSP (Sirene + MidiIn), compiled without
// JUCE (build.rs) and driven sample by sample the way SirenVoiceUnit::process() drives it, behind
// a C ABI (cso_*) that src/lib.rs wraps.
//
// SirenVoiceUnit is not used directly because it keeps its Sirene private, and the tests need
// changeQualite, setisCrossFade and set16ou8Bit.

#include <memory>
#include <string>

#include "lib/dsp/MidiIn.h"
#include "lib/dsp/Sirene.h"

namespace {

struct Voice {
    std::unique_ptr<Sirene> siren;
    std::unique_ptr<MidiIn> midiIn;
    int maxNote = 127;
    int timerCount = 0;
    int setNoteCount = 0;
    int setNotePeriod = 44;

    Voice(sirenId id, const std::string& resources, double sampleRate) {
        siren = std::make_unique<Sirene>(id, resources);
        midiIn = std::make_unique<MidiIn>(
            id,
            [this](float v) { siren->setVelocite(v); },
            [this](float v) { siren->setnoteFromExt(v); });
        // same clipping as SirenVoiceUnit
        const auto& data = sirenPropertiesById.at(id);
        maxNote = std::get<1>(data->velocityRanges.rbegin()->second) + 12;
        setNotePeriod = static_cast<int>(sampleRate / 1000.0f);
        midiIn->setSampleRate(sampleRate);
        siren->setSampleRate(sampleRate);
    }

    void noteOn(int note, int velocity) {
        midiIn->realTimeStartNote(note < maxNote ? note : maxNote, velocity);
    }

    void noteOff(int note) {
        midiIn->realTimeStopNote(note < maxNote ? note : maxNote);
    }

    // SirenVoiceUnit::process(), verbatim
    float process() {
        if (timerCount % 512 == 0) {
            midiIn->timerAudio();
            timerCount = 0;
        }
        ++timerCount;
        if (setNoteCount % setNotePeriod == 0) {
            siren->setnote();
            setNoteCount = 0;
        }
        ++setNoteCount;
        return siren->calculwave();
    }
};

} // namespace


// --- C ABI ---------------------------------------------------------------------
// A voice as the plugin builds it: CC 7 = 127 already sent, quality 30,
// crossfade off, 8-bit on. Null on a bad siren id. No exception crosses.

extern "C" {

void* cso_voice_new(const char* resources, const char* siren, double sampleRate) {
    try {
        std::string r = resources ? resources : "";
        if (!r.empty() && r.back() != '/') r += '/';
        auto it = sirenIdByStrId.find(siren ? siren : "");
        if (it == sirenIdByStrId.end()) return nullptr;
        auto* v = new Voice(it->second, r, sampleRate);
        v->midiIn->handleControlChange(7, 127);
        return v;
    } catch (...) {
        return nullptr;
    }
}

void cso_voice_free(void* v) { delete static_cast<Voice*>(v); }
void cso_voice_set_quality(void* v, int q) { static_cast<Voice*>(v)->siren->changeQualite(q); }
void cso_voice_set_crossfade(void* v, int on) { static_cast<Voice*>(v)->siren->setisCrossFade(on); }
// set16ou8Bit(true) means 8-bit
void cso_voice_set_8bit(void* v, int on) { static_cast<Voice*>(v)->siren->set16ou8Bit(on != 0); }
void cso_voice_note_on(void* v, int note, int velocity) { static_cast<Voice*>(v)->noteOn(note, velocity); }
void cso_voice_note_off(void* v, int note) { static_cast<Voice*>(v)->noteOff(note); }
void cso_voice_control_change(void* v, int cc, int value) { static_cast<Voice*>(v)->midiIn->handleControlChange(cc, value); }
void cso_voice_pitch_wheel(void* v, int lsb, int msb) { static_cast<Voice*>(v)->midiIn->handlePitchWheel(lsb, msb); }
float cso_voice_pitch(void* v) { return static_cast<Voice*>(v)->siren->getCurrentPitch(); }

void cso_voice_render(void* v, float* out, long n) {
    auto* voice = static_cast<Voice*>(v);
    for (long i = 0; i < n; ++i) out[i] = voice->process();
}

} // extern "C"

