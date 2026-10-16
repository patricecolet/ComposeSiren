// DSP Control Midi Input (COMPOSESIREN_DSP_CONTROL_MIDI_IN): PR #16 CC mapping on the extra port,
// never mixed with MidiIn (siren firmware CCs).
// Usage: SirenOrchestraDspControlMidiInTests

#include "../DspControlMidiInput.h"
#include "../DspControlMidiRouter.h"
#include "PerSirenMidiBridges.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

void check(bool ok, const char* what)
{
    std::cout << (ok ? "ok   " : "FAIL ") << what << "\n";
    if (!ok)
        throw std::runtime_error(what);
}

struct Proc : juce::AudioProcessor {
    std::vector<parameterLayoutGroupData> groups;
    juce::AudioProcessorValueTreeState apvts;

    static std::vector<parameterLayoutGroupData> makeGroups()
    {
        std::vector<parameterLayoutGroupData> res;
        for (const auto sid : allSirenIds)
            res.push_back(mkLayoutGroupData(sirenStrIdById.at(sid),
                                            sirenTitleById.at(sid),
                                            ParameterClass::SirenControl,
                                            ParameterClass::TrackControl));
        res.push_back(mkLayoutGroupData("R", "Reverb", ParameterClass::ReverbControl));
        res.push_back(mkLayoutGroupData("M", "Master", ParameterClass::MasterControl));
        return res;
    }

    Proc()
        : groups(makeGroups())
        , apvts(*this, nullptr, "P", createParameterLayout(groups))
    {
    }

    const juce::String getName() const override { return "dsp-control-midi-in-test"; }
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
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

juce::RangedAudioParameter& param(Proc& p, const char* group, ParameterId id)
{
    auto* q = p.apvts.getParameter(ParameterIdGet::toJuceParameterId(group, id));
    if (q == nullptr)
        throw std::runtime_error("missing parameter");
    return *q;
}

float native(Proc& p, const char* group, ParameterId id)
{
    auto& q = param(p, group, id);
    return q.convertFrom0to1(q.getValue());
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    auto run = [&](auto fn, const char* name) {
        try {
            fn();
        } catch (const std::exception& e) {
            std::cout << "FAIL " << name << ": " << e.what() << "\n";
            ++failures;
        }
    };

    run([] {
        check(DspControlMidiInput::virtualDestinationName(0)
                  == "SirenOrchestra: DSP Control",
              "first instance is unnumbered");
        check(DspControlMidiInput::virtualDestinationName(1)
                  == "SirenOrchestra 2: DSP Control",
              "second instance numbers the SirenOrchestra prefix");
        check(DspControlMidiInput::virtualDestinationName(2)
                  == "SirenOrchestra 3: DSP Control",
              "third instance");
    }, "virtual destination names");

    run([] {
        Proc p;
        juce::MidiKeyboardState kbs;
        PerSirenMidiBridges bridges(p.groups.front(), p.apvts, kbs);
        const float panBefore = native(p, "S1", ParameterId::TrackPanning);
        MidiScheduler scheduler;
        bridges.handleMessage(scheduler, juce::MidiMessage::controllerEvent(1, 10, 0), 0);
        bridges.handleMessage(scheduler, juce::MidiMessage::controllerEvent(1, 70, 0), 0);
        bridges.handleMessage(scheduler, juce::MidiMessage::controllerEvent(1, 7, 64), 0);
        check(std::abs(native(p, "S1", ParameterId::TrackPanning) - panBefore) < 1e-6f,
              "siren MIDI path does not move pan (CC 10)");
        check(std::abs(native(p, "S1", ParameterId::TrackOutputGain) - 1.0f) < 1e-6f,
              "siren MIDI path does not move gain (CC 70)");
        check(std::abs(native(p, "S1", ParameterId::Volume) - 64.0f) < 0.5f,
              "siren MIDI path still handles volume (CC 7)");
    }, "PerSirenMidiBridges ignores mixer CCs");

    run([] {
        Proc p;
        DspControlMidiRouter router(p.groups, p.apvts);

        check(router.handle(juce::MidiMessage::controllerEvent(1, 10, 0)), "ch1 CC10 pan is DSP MIDI");
        check(std::abs(native(p, "S1", ParameterId::TrackPanning) + 1.0f) < 1e-4f, "CC 10 = 0 is hard left");
        check(router.handle(juce::MidiMessage::controllerEvent(1, 10, 127)), "ch1 CC10 max");
        check(std::abs(native(p, "S1", ParameterId::TrackPanning) - 1.0f) < 1e-4f, "CC 10 = 127 is hard right");
        check(router.handle(juce::MidiMessage::controllerEvent(7, 70, 0)), "ch7 CC70 gain is DSP MIDI");
        check(std::abs(native(p, "S7", ParameterId::TrackOutputGain)) < 1e-4f, "CC 70 = 0 is silence");
        check(router.handle(juce::MidiMessage::controllerEvent(7, 70, 127)), "ch7 CC70 max");
        check(std::abs(native(p, "S7", ParameterId::TrackOutputGain) - 1.0f) < 1e-4f, "CC 70 = 127 is unity");

        const float vol = native(p, "S1", ParameterId::Volume);
        check(!router.handle(juce::MidiMessage::controllerEvent(1, 7, 0)), "ch1 CC7 stays with MidiIn");
        check(std::abs(native(p, "S1", ParameterId::Volume) - vol) < 1e-6f, "siren volume unchanged");
        check(!router.handle(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100)), "notes stay with MidiIn");
        check(!router.handle(juce::MidiMessage::controllerEvent(16, 121, 127)), "CC 121 stays with MidiIn");
    }, "per-siren pan/gain mapping");

    run([] {
        Proc p;
        DspControlMidiRouter router(p.groups, p.apvts);

        check(router.handle(juce::MidiMessage::controllerEvent(16, 7, 127)), "ch16 CC7 master");
        check(std::abs(native(p, "M", ParameterId::MasterVolume) - 1.0f) < 1e-4f, "master CC 127 is unity");
        check(router.handle(juce::MidiMessage::controllerEvent(16, 7, 0)), "master zero");
        check(std::abs(native(p, "M", ParameterId::MasterVolume)) < 1e-4f, "master CC 0 is silence");

        check(router.handle(juce::MidiMessage::controllerEvent(16, 64, 63)), "reverb off");
        check(native(p, "R", ParameterId::ReverbEnable) < 0.5f, "CC 64 < 64 is off");
        check(router.handle(juce::MidiMessage::controllerEvent(16, 64, 64)), "reverb on");
        check(native(p, "R", ParameterId::ReverbEnable) > 0.5f, "CC 64 >= 64 is on");

        check(router.handle(juce::MidiMessage::controllerEvent(16, 65, 127)), "room size");
        check(std::abs(native(p, "R", ParameterId::ReverbRoomSize) - 1.0f) < 1e-4f, "CC 65 = 127 is full size");
        check(router.handle(juce::MidiMessage::controllerEvent(16, 66, 64)), "dry/wet");
        check(std::abs(native(p, "R", ParameterId::ReverbDryWet) - 64.0f / 127.0f) < 1e-4f, "CC 66 maps 0-1");
        check(router.handle(juce::MidiMessage::controllerEvent(16, 67, 0)), "damp");
        check(std::abs(native(p, "R", ParameterId::ReverbDamping)) < 1e-4f, "CC 67 = 0");
        check(router.handle(juce::MidiMessage::controllerEvent(16, 70, 127)), "width");
        check(std::abs(native(p, "R", ParameterId::ReverbWidth) - 1.0f) < 1e-4f, "CC 70 on ch16 is width");
        check(router.handle(juce::MidiMessage::controllerEvent(16, 68, 0)), "low cut");
        check(std::abs(native(p, "R", ParameterId::ReverbLowCut) - 20.0f) < 0.5f, "CC 68 = 0 is 20 Hz");
        check(router.handle(juce::MidiMessage::controllerEvent(16, 69, 127)), "high cut");
        check(std::abs(native(p, "R", ParameterId::ReverbHighCut) - 20000.0f) < 1.0f, "CC 69 = 127 is 20 kHz");
    }, "channel 16 reverb and master");

    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
