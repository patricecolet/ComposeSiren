// The reference for siren-dsp's tests: the plugin's own DSP (Sirene + MidiIn), compiled without
// JUCE (build.rs) and driven sample by sample the way SirenVoiceUnit::process() drives it, behind
// a C ABI (cso_*) that src/lib.rs wraps.
//
// SirenVoiceUnit is not used directly because it keeps its Sirene private, and the tests need
// changeQualite, setisCrossFade and set16ou8Bit.

#include <memory>
#include <string>

#include "Reverb.h"
#include "lib/dsp/MidiIn.h"
#include "lib/dsp/Sirene.h"
#include "lib/dsp/mareverbe.h"

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

// --- mareverbe -----------------------------------------------------------------

void* cso_mareverbe_new() {
    try {
        return new mareverbe();
    } catch (...) {
        return nullptr;
    }
}

void cso_mareverbe_free(void* m) { delete static_cast<mareverbe*>(m); }
void cso_mareverbe_mute(void* m) { static_cast<mareverbe*>(m)->mute(); }
void cso_mareverbe_set_room_size(void* m, float v) { static_cast<mareverbe*>(m)->setroomsize(v); }
float cso_mareverbe_get_room_size(void* m) { return static_cast<mareverbe*>(m)->getroomsize(); }
void cso_mareverbe_set_damp(void* m, float v) { static_cast<mareverbe*>(m)->setdamp(v); }
float cso_mareverbe_get_damp(void* m) { return static_cast<mareverbe*>(m)->getdamp(); }
void cso_mareverbe_set_wet(void* m, float v) { static_cast<mareverbe*>(m)->setwet(v); }
float cso_mareverbe_get_wet(void* m) { return static_cast<mareverbe*>(m)->getwet(); }
void cso_mareverbe_set_dry(void* m, float v) { static_cast<mareverbe*>(m)->setdry(v); }
float cso_mareverbe_get_dry(void* m) { return static_cast<mareverbe*>(m)->getdry(); }
void cso_mareverbe_set_width(void* m, float v) { static_cast<mareverbe*>(m)->setwidth(v); }
float cso_mareverbe_get_width(void* m) { return static_cast<mareverbe*>(m)->getwidth(); }
void cso_mareverbe_set_mode(void* m, float v) { static_cast<mareverbe*>(m)->setmode(v); }
float cso_mareverbe_get_mode(void* m) { return static_cast<mareverbe*>(m)->getmode(); }
void cso_mareverbe_process_stereo(void* m, float* inL, float* inR, float* outL, float* outR, long n) {
    static_cast<mareverbe*>(m)->process_stereo(inL, inR, outL, outR, n);
}

// --- Reverb --------------------------------------------------------------------

void* cso_reverb_new() {
    try {
        return new Reverb();
    } catch (...) {
        return nullptr;
    }
}

void cso_reverb_free(void* r) { delete static_cast<Reverb*>(r); }
void cso_reverb_set_sample_rate(void* r, double sr) { static_cast<Reverb*>(r)->setSampleRate(sr); }
void cso_reverb_begin_process_block(void* r) { static_cast<Reverb*>(r)->beginProcessBlock(); }
void cso_reverb_enable(void* r, int e) { static_cast<Reverb*>(r)->enable(e != 0); }
void cso_reverb_set_dry_wet(void* r, float v) { static_cast<Reverb*>(r)->setDryWet(v); }
void cso_reverb_set_damping(void* r, float v) { static_cast<Reverb*>(r)->setDamping(v); }
void cso_reverb_set_room_size(void* r, float v) { static_cast<Reverb*>(r)->setRoomSize(v); }
void cso_reverb_set_width(void* r, float v) { static_cast<Reverb*>(r)->setWidth(v); }
void cso_reverb_set_low_cut(void* r, float v) { static_cast<Reverb*>(r)->setLowCut(v); }
void cso_reverb_set_high_cut(void* r, float v) { static_cast<Reverb*>(r)->setHighCut(v); }
void cso_reverb_process(void* r, float* inL, float* inR, float* outL, float* outR, int n) {
    static_cast<Reverb*>(r)->process(inL, inR, outL, outR, n);
}

} // extern "C"

