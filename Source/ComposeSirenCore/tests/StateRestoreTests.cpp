// A reloaded plugin (the host reopens the project: a new processor, then setStateInformation)
// must play and show what was saved:
// - a MIDI bridge asked to resend (sendAllCurrentParameterValues) sends the parameter's
//   current value, a restored one included, not the value it saw when it was made;
// - the reverb switch shows the restored ReverbEnable when the editor opens, and follows
//   later changes;
// - controllers that arrive while a siren is still loading (no unit to play them) reach the
//   siren once loaded (COMPOSESIREN_RUST_DSP): it plays, bit for bit, as a voice given the
//   same controllers directly.
// Usage: ComposeSirenStateRestoreTests [<resources folder, ending with />]

#include "PerSirenMidiBridges.h"
#include "Components/ReverbStripComponent.h"
#include "lib/wrappers/SirenVoice.h"

#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

namespace {

struct Proc : juce::AudioProcessor {
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParameterIdGet::toJuceParameterId("S1", ParameterId::Volume), 1 },
            "Volume", 0.0f, 127.0f, 100.0f));
        for (auto id : { ParameterId::ReverbEnable, ParameterId::ReverbDryWet, ParameterId::ReverbDamping,
                         ParameterId::ReverbRoomSize, ParameterId::ReverbWidth, ParameterId::ReverbLowCut,
                         ParameterId::ReverbHighCut })
            l.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { ParameterIdGet::toJuceParameterId("R", id), 1 },
                ParameterIdGet::codeName(id), 0.0f, 1.0f, 0.0f));
        return l;
    }
    Proc() : apvts(*this, nullptr, "P", layout()) {}
    const juce::String getName() const override { return "test"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& d) override {
        if (auto x = apvts.copyState().createXml()) copyXmlToBinary(*x, d);
    }
    void setStateInformation(const void* d, int n) override {
        if (auto x = getXmlFromBinary(d, n)) apvts.replaceState(juce::ValueTree::fromXml(*x));
    }
};

juce::RangedAudioParameter& param(Proc& p, const char* group, ParameterId id) {
    return *p.apvts.getParameter(ParameterIdGet::toJuceParameterId(group, id));
}

void collectToggles(juce::Component& c, std::vector<juce::Button*>& out) {
    for (auto* ch : c.getChildren()) {
        if (auto* b = dynamic_cast<juce::ToggleButton*>(ch)) out.push_back(b);
        collectToggles(*ch, out);
    }
}

bool allToggles(juce::Component& c, bool on) {
    std::vector<juce::Button*> t;
    collectToggles(c, t);
    if (t.empty()) return false;
    for (auto* b : t) if (b->getToggleState() != on) return false;
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    auto check = [&](bool ok, const std::string& what) {
        std::cout << (ok ? "ok   " : "FAIL ") << what << "\n";
        if (!ok) ++failures;
    };

    // the saved project: Volume 30, reverb on
    juce::MemoryBlock saved;
    {
        Proc a;
        param(a, "S1", ParameterId::Volume).setValueNotifyingHost(30.0f / 127.0f);
        param(a, "R", ParameterId::ReverbEnable).setValueNotifyingHost(1.0f);
        a.getStateInformation(saved);
    }

    // reload: a new processor whose bridge was made before the state came back
    Proc b;
    ParameterMidiBridge<CCParam> volume(param(b, "S1", ParameterId::Volume), OneBasedMidiChannel { 1 }, 7, true);
    b.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    {
        MidiScheduler sched;
        volume.processBlock(sched, 4096); // whatever the listener pushed
        volume.setPending();             // sendAllCurrentParameterValues
        volume.processBlock(sched, 4096);
        juce::MidiBuffer out;
        sched.flush(out);
        int last = -1;
        for (const auto m : out) if (m.getMessage().isController()) last = m.getMessage().getControllerValue();
        check(last == 30, "bridge resends the restored value (CC 7 = " + std::to_string(last) + ", saved 30)");
    }
    {
        // a value the bridge never heard of (set without notifying): a resend must still send it
        param(b, "S1", ParameterId::Volume).setValue(param(b, "S1", ParameterId::Volume).convertTo0to1(64.0f));
        MidiScheduler sched;
        volume.setPending();
        volume.processBlock(sched, 4096);
        juce::MidiBuffer out;
        sched.flush(out);
        int last = -1;
        for (const auto m : out) if (m.getMessage().isController()) last = m.getMessage().getControllerValue();
        check(last == 64, "bridge resends the parameter's current value, not its last seen (CC 7 = " + std::to_string(last) + ", now 64)");
    }

    // the editor opens after the restore: the reverb switch shows it, then follows changes
    {
        ReverbStripComponent strip(b.apvts, "R");
        check(allToggles(strip, true), "reverb switch on after a restore with the reverb on");
        param(b, "R", ParameterId::ReverbEnable).setValueNotifyingHost(0.0f);
        check(allToggles(strip, false), "reverb switch follows the parameter (turned off)");
    }

#if COMPOSESIREN_RUST_DSP
    if (argc >= 2) {
        const std::string res = argv[1];
        const double rate = 48000.0;
        const std::vector<std::array<int, 3>> ccs { { 0xB2, 7, 30 }, { 0xB2, 1, 90 }, { 0xB2, 9, 70 }, { 0xE2, 0, 80 } };
        cs::setDspChoice({ "cpp", "fixed" });
        SirenVoice v;
        // the controllers arrive while the voice has no siren yet (still loading)
        check(!v.getRawSirenHandle(), "no siren before the load");
        for (auto& c : ccs) v.handleMidiWithoutHandle(c[0], c[1], c[2]);
        v.setSirenId(S3, res);
        v.setSampleRate(rate);
        SirenVoiceUnit ref(S3, res, cs::DspChoice { "cpp", "fixed" });
        ref.setSampleRate(rate);
        for (auto& c : ccs) ref.handleMidi(c[0], c[1], c[2]);
        std::vector<float> a, r;
        for (int b2 = 0; b2 < 200; ++b2) {
            v.getRawSirenHandle();
            v.beginProcessBlock();
            ref.beginProcessBlock();
            for (int i = 0; i < 512; ++i) {
                if (b2 == 0 && i == 0) { v.handleMidi(0x92, 60, 100); ref.handleMidi(0x92, 60, 100); }
                a.push_back(v.process());
                r.push_back(ref.process());
            }
            v.collectGarbage();
        }
        float diff = 0;
        for (size_t i = 0; i < a.size(); ++i) diff = std::max(diff, std::abs(a[i] - r[i]));
        check(diff == 0.0f, "controllers sent while loading reach the siren (max |diff| " + std::to_string(diff) + ")");
    }
#endif

    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
