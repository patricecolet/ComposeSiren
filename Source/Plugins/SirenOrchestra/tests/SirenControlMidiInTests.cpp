// Siren Control Midi Input (COMPOSESIREN_SIREN_CONTROL_MIDI_IN): extra port
// joins MidiIn (notes, volume, vibrato). Same virtual-destination convention
// as DSP Control ("SirenOrchestra: <role>", numbered prefix for instances).
// Usage: SirenOrchestraSirenControlMidiInTests

#include "../SirenControlMidiInput.h"
#include "../DspControlMidiInput.h"
#include "../OrchestraMidiRouter.h"
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

    const juce::String getName() const override { return "siren-control-midi-in-test"; }
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
        check(SirenControlMidiInput::virtualDestinationName(0)
                  == "SirenOrchestra: Siren Control",
              "first instance is unnumbered");
        check(SirenControlMidiInput::virtualDestinationName(1)
                  == "SirenOrchestra 2: Siren Control",
              "second instance numbers the SirenOrchestra prefix");
        check(SirenControlMidiInput::virtualDestinationName(2)
                  == "SirenOrchestra 3: Siren Control",
              "third instance");
        check(DspControlMidiInput::virtualDestinationName(0)
                  == "SirenOrchestra: DSP Control",
              "DSP Control keeps the same convention");
        check(VirtualMidiInput::virtualDestinationName("Siren Control", 0)
                  == SirenControlMidiInput::virtualDestinationName(0),
              "role helper matches the Siren Control wrapper");
    }, "virtual destination names");

    run([] {
        Proc p;
        juce::MidiKeyboardState kbs;
        OrchestraMidiRouter router(p.groups, p.apvts, kbs);
        MidiScheduler scheduler;

        juce::MidiBuffer extra;
        extra.addEvent(juce::MidiMessage::controllerEvent(1, 7, 0), 0);
        extra.addEvent(juce::MidiMessage::controllerEvent(1, 10, 0), 1);
        juce::MidiBuffer midiIn;
        for (const auto metadata : extra)
            midiIn.addEvent(metadata.getMessage(), metadata.samplePosition);

        const float panBefore = native(p, "S1", ParameterId::TrackPanning);
        for (const auto metadata : midiIn)
            router.handleMessage(scheduler, metadata.getMessage(), metadata.samplePosition);

        check(std::abs(native(p, "S1", ParameterId::Volume)) < 0.5f,
              "merged extra MIDI moves volume (CC 7)");
        check(std::abs(native(p, "S1", ParameterId::TrackPanning) - panBefore) < 1e-6f,
              "merged extra MIDI does not move pan (CC 10 stays with DSP Control)");
    }, "extra port joins MidiIn");

    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
