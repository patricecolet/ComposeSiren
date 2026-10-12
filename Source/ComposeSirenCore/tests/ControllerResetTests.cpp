#include "lib/dsp/MidiIn.h"
#include "lib/settings/Settings.h"
#include "../../Plugins/SirenOrchestra/OrchestraMidiRouter.h"
#include "../../Plugins/OneSiren/OneMidiRouter.h"

#include <iostream>
#include <stdexcept>

namespace {

void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void checkMidi(bool enabled)
{
    float volume = -1;
    MidiIn midi(allSirenIds.front(), [&](float v) { volume = v; }, [](float) {});
    midi.setControllerResetEnabled(enabled);
    auto play = [&] {
        midi.handleControlChange(7, 127);
        midi.realTimeStartNote(69, 100);
        check(volume > 0, "note must sound before reset");
    };

    play();
    midi.handleControlChange(120, 0);
    check(volume > 0, "characterize the current CC 120 value-zero discrepancy");
    midi.handleControlChange(120, 1);
    check(enabled ? volume == 0 : volume > 0, "CC 120 must obey the runtime switch");

    play();
    midi.handleControlChange(123, 0);
    check(volume > 0, "characterize the current CC 123 value-zero discrepancy");
    midi.handleControlChange(123, 1);
    check(enabled ? volume == 0 : volume > 0, "CC 123 must obey the runtime switch");

    for (int value : {0, 127}) {
        play();
        const auto before = volume;
        midi.handleControlChange(121, value);
        check(enabled ? volume == 0 : volume == before,
              "CC 121 keeps master behaviour when disabled, for any value");
        for (int i = 0; i < 200; ++i) midi.timerAudio();
        check(enabled ? volume == 0 : volume == before, "reset must not restart the volume ramp");
    }

    // CC 123 follows the release envelope rather than cutting immediately.
    play();
    midi.handleControlChange(72, 20);
    midi.handleControlChange(123, 1);
    check(volume > 0, "release does not cut immediately");
    for (int i = 0; i < 1000; ++i) midi.timerAudio();
    check(enabled ? volume == 0 : volume > 0, "release completes only when enabled");
}

void checkSettings()
{
    const auto& row = cs::Settings::describe(cs::Settings::controllerResetEnabled);
    check(row.id == "reset_all_controllers.enabled", "stable setting storage key");
    check(row.group == "Controller reset" && row.type == mecaviv::metadata::SettingType::Bool,
          "one boolean in the reset section");
    check(row.defaultValue == 1.0 && row.scope == "global", "enabled by default, shared per process");
    check(cs::Settings::isAvailable(cs::Settings::controllerResetEnabled), "setting is available");
    check(cs::Settings::settingCount == mecaviv::metadata::settingCount + 10, "ten optional rows");
    for (auto id : { cs::Settings::dspAllSoundsOffValues, cs::Settings::dspResetControllersValues,
                     cs::Settings::dspAllNotesOffValues }) {
        const auto& choice = cs::Settings::describe(id);
        check(choice.type == mecaviv::metadata::SettingType::Choice && choice.maximum == 3,
              "four DSP value interpretations");
        check(cs::Settings::isAvailable(id), "DSP choice is available");
    }
    check(cs::Settings::describe(cs::Settings::dspAllSoundsOffValues).defaultValue == 2
          && cs::Settings::describe(cs::Settings::dspResetControllersValues).defaultValue == 3
          && cs::Settings::describe(cs::Settings::dspAllNotesOffValues).defaultValue == 2,
          "persisted defaults preserve the original experiment");
    for (auto id : { cs::Settings::bridgeAllSoundsOffValues, cs::Settings::bridgeResetControllersValues,
                     cs::Settings::bridgeAllNotesOffValues, cs::Settings::bridgeAllSoundsOffOutput,
                     cs::Settings::bridgeResetControllersOutput, cs::Settings::bridgeAllNotesOffOutput })
        check(cs::Settings::isAvailable(id) == (COMPOSESIREN_PARK_BRIDGE != 0),
              "bridge choices require a compiled bridge");
}

void checkValueRules()
{
    // The expected outcomes are independent of the policy helper's logic.
    const bool accepted[4][3] = { {false, false, false}, {true, false, false},
                                 {false, true, true}, {true, true, true} };
    const int values[] = {0, 1, 127};
    for (int rule = 0; rule < 4; ++rule) {
        cs::DspChannelModePolicy policy;
        policy.values.fill(static_cast<cs::MidiValueRule>(rule));
        policy = cs::DspChannelModePolicy::unpack(policy.packed());
        for (int cc : {120, 121, 123}) {
            for (int v = 0; v < 3; ++v) {
                float volume = -1;
                MidiIn midi(allSirenIds.front(), [&](float n) { volume = n; }, [](float) {});
                midi.setChannelModePolicy(policy);
                midi.handleControlChange(7, 127);
                midi.realTimeStartNote(69, 100);
                midi.handleControlChange(cc, values[v]);
                check(accepted[rule][v] ? volume == 0 : volume > 0,
                      "each DSP operation must obey its selected value rule");
            }
        }
    }

    cs::DspChannelModePolicy policy;
    policy.values = {cs::MidiValueRule::zero, cs::MidiValueRule::ignore, cs::MidiValueRule::any};
    check(policy.accepts(120, 0) && !policy.accepts(120, 1) && !policy.accepts(121, 0)
          && policy.accepts(123, 0) && policy.accepts(123, 127), "rules are independent per controller");
    policy.enabled = false;
    policy = cs::DspChannelModePolicy::unpack(policy.packed());
    check(!policy.accepts(120, 0) && !policy.accepts(123, 127)
          && policy.accepts(121, 0) && policy.accepts(121, 127), "disabled feature bypasses custom rules");

    // ASO interrupts an in-progress release without resetting controller values.
    float volume = -1;
    MidiIn midi(allSirenIds.front(), [&](float n) { volume = n; }, [](float) {});
    midi.handleControlChange(7, 64);
    midi.realTimeStartNote(69, 100);
    const auto originalVolume = volume;
    midi.handleControlChange(72, 20);
    midi.handleControlChange(123, 1);
    check(volume > 0, "ANO uses the normal release envelope");
    midi.handleControlChange(120, 1);
    for (int i = 0; i < 1000; ++i) midi.timerAudio();
    check(volume == 0, "ASO cancels release and cannot restart the volume");
    midi.realTimeStartNote(69, 100);
    check(volume == originalVolume, "ASO keeps the volume controller for the next note");
}

void checkBridgeRules()
{
    for (int cc : {120, 121, 123}) {
        for (int value : {0, 1, 127}) {
            cs::BridgeChannelModePolicy policy;
            check(policy.forwardedValue(cc, value) == value, "bridge defaults preserve every value");
            for (int rule = 0; rule < 4; ++rule) {
                for (int format = 0; format < 3; ++format) {
                    policy.values.fill(static_cast<cs::MidiValueRule>(rule));
                    policy.output.fill(static_cast<cs::MidiOutputValue>(format));
                    policy = cs::BridgeChannelModePolicy::unpack(policy.packed());
                    const auto output = policy.forwardedValue(cc, value);
                    const bool accepted = rule == 3 || (rule == 1 && value == 0) || (rule == 2 && value > 0);
                    check(output.has_value() == accepted, "filter the original value before converting it");
                    if (accepted)
                        check(*output == (format == 0 ? value : format == 1 ? 0 : 127),
                              "accepted bridge messages get the chosen value");
                    check(policy.forwardedValue(7, 64) == 64, "other controllers pass through");
                }
            }
            policy.enabled = false;
            policy = cs::BridgeChannelModePolicy::unpack(policy.packed());
            check(policy.forwardedValue(cc, value) == value, "disabled feature bypasses bridge conversion");
        }
    }
    cs::BridgeChannelModePolicy bridge;
    bridge.values = {cs::MidiValueRule::zero, cs::MidiValueRule::positive, cs::MidiValueRule::ignore};
    bridge.output = {cs::MidiOutputValue::positive, cs::MidiOutputValue::zero, cs::MidiOutputValue::preserve};
    bridge = cs::BridgeChannelModePolicy::unpack(bridge.packed());
    check(bridge.forwardedValue(120, 0) == 127 && !bridge.forwardedValue(120, 1)
          && bridge.forwardedValue(121, 127) == 0 && !bridge.forwardedValue(121, 0)
          && !bridge.forwardedValue(123, 1), "bridge rules and output values are independent per controller");
}

class TestProcessor : public juce::AudioProcessor {
public:
    const juce::String getName() const override { return "Test"; }
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

void checkKnobRouting()
{
    TestProcessor processor;
    const auto sid = allSirenIds.front();
    const auto group = mkLayoutGroupData(sirenStrIdById.at(sid), "Siren", ParameterClass::SirenControl);
    std::vector<parameterLayoutGroupData> groups {group};
    juce::AudioProcessorValueTreeState vts(processor, nullptr, "Test", createParameterLayout(groups));
    juce::MidiKeyboardState keyboard;
    OneMidiRouter one(group, vts, keyboard);
    OrchestraMidiRouter orchestra(groups, vts, keyboard);
    auto* knob = vts.getParameter(ParameterIdGet::toJuceParameterId(group.id, ParameterId::VibratoAmplitude));
    check(knob != nullptr, "vibrato knob exists");
    const auto channel = sirenPropertiesById.at(sid)->oneBasedMidiChannel.oneBased;
    for (int route = 0; route < 3; ++route) {
        for (auto rule : {cs::MidiValueRule::ignore, cs::MidiValueRule::zero,
                          cs::MidiValueRule::positive, cs::MidiValueRule::any}) {
            for (int value : {0, 1, 127}) {
                for (bool enabled : {false, true}) {
                    cs::DspChannelModePolicy policy;
                    policy.enabled = enabled;
                    policy.values[1] = rule;
                    one.setChannelModePolicy(policy);
                    orchestra.setChannelModePolicy(policy);
                    knob->setValueNotifyingHost(0.75f);
                    const auto before = knob->getValue();
                    MidiScheduler scheduler;
                    const auto msg = juce::MidiMessage::controllerEvent(route == 2 ? 16 : channel, 121, value);
                    if (route == 0) one.handleMessage(scheduler, msg, 17);
                    else orchestra.handleMessage(scheduler, msg, 17);
                    const bool resets = !enabled || rule == cs::MidiValueRule::any
                        || (rule == cs::MidiValueRule::zero && value == 0)
                        || (rule == cs::MidiValueRule::positive && value > 0);
                    check(knob->getValue() == (resets ? 0.0f : before),
                          "knob reset follows the same CC 121 rule as the DSP, including channel 16");
                    juce::MidiBuffer output;
                    scheduler.flush(output);
                    check(output.getNumEvents() == 1, "knob sync emits no extra CCs");
                    const auto event = *output.begin();
                    const auto routedValue = enabled ? value : std::min(value, 1);
                    check(event.samplePosition == 17 && event.getMessage().getControllerValue() == routedValue
                          && event.getMessage().getChannel() == channel,
                          "DSP filtering preserves routed MIDI for independent bridge interpretation");
                }
            }
        }
    }
}

} // namespace

int main()
{
    try {
        checkMidi(true);
        checkMidi(false);
        checkSettings();
        checkValueRules();
        checkBridgeRules();
        checkKnobRouting();
        std::cout << "Controller reset runtime and MIDI characterization checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
