#include "Settings.h"

#ifndef COMPOSESIREN_RUST_DSP
#define COMPOSESIREN_RUST_DSP 0
#endif
#if COMPOSESIREN_RUST_DSP
#include "../wrappers/DspChoice.h"
#endif

#ifndef COMPOSESIREN_SIREN_WAVES
#define COMPOSESIREN_SIREN_WAVES 0
#endif
#ifndef COMPOSESIREN_DSP_MIDI
#define COMPOSESIREN_DSP_MIDI 0
#endif

namespace cs {

namespace meta = mecaviv::metadata;

#if COMPOSESIREN_RUST_DSP
namespace {
// the choice at `index` of a Choice setting, as its id
std::string choiceAt(const meta::Setting& s, int index)
{
    std::string_view rest = s.choices;
    for (int i = 0;; ++i) {
        const auto bar = rest.find('|');
        if (i == index) return std::string(rest.substr(0, bar));
        if (bar == std::string_view::npos) return {};
        rest.remove_prefix(bar + 1);
    }
}
} // namespace

// the dsp.* settings, for the voices (DspChoice.h)
static void publishDspChoice(const Settings& s)
{
    DspChoice c;
    c.variant = choiceAt(Settings::describe(meta::SettingId::dspVariant), s.getInt(meta::SettingId::dspVariant));
    c.dataset = choiceAt(Settings::describe(meta::SettingId::dspDataset), s.getInt(meta::SettingId::dspDataset));
    c.skipSilent = s.getBool(meta::SettingId::dspSkipSilentFrames);
    setDspChoice(c);
}
#endif

Settings::Settings()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "ComposeSiren";
    options.filenameSuffix = ".settings";
    options.folderName = "Mecanique Vivante";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 500;
    file = std::make_unique<juce::PropertiesFile>(options);

    for (std::size_t i = 0; i < settingCount; ++i) {
        const auto id = idAt(i);
        juce::var v = describe(id).type == meta::SettingType::String
                          ? juce::var(juce::String())
                          : juce::var(describe(id).defaultValue);
        if (file->containsKey(key(id).toString())) {
            if (describe(id).type == meta::SettingType::String)
                v = file->getValue(key(id).toString());
            else
                v = file->getValue(key(id).toString()).getDoubleValue();
        }
        tree.setProperty(key(id), normalised(id, v), nullptr);
    }
    tree.addListener(this);
#if COMPOSESIREN_RUST_DSP
    publishDspChoice(*this);
#endif
}

Settings::~Settings()
{
    tree.removeListener(this);
    file->saveIfNeeded();
}

const meta::Setting& Settings::describe(Id id)
{
#if COMPOSESIREN_RESETALLCONTROLLERS
    if (isControllerResetSetting(id)) {
        static constexpr std::string_view receiveChoices = "Ignore|Zero only|Positive only|Any value";
        static constexpr std::string_view outputChoices = "Preserve|Send 0|Send 127";
        static constexpr std::string_view dspDescription =
            "Which incoming values trigger this DSP operation. MIDI specifies value 0; "
            "positive-only supports momentary buttons without acting on their release.";
        static constexpr std::string_view bridgeDescription =
            "Which incoming values are forwarded to the bridge, independently of the DSP. "
            "Filtering happens before the output value is converted.";
        static constexpr std::string_view outputDescription =
            "Value sent for accepted messages, on the bridge copy only. "
            "Conversion does not add support for a command at the receiver.";
        static constexpr std::array<meta::Setting, 10> reset {{ {
            "reset_all_controllers.enabled", "Controller reset", "Enable",
            meta::SettingType::Bool, 1.0, 0.0, 1.0, "", "",
            "COMPOSESIREN_RESETALLCONTROLLERS", "global",
            "Enable the experimental controller reset and MIDI CC 120/123 handling. "
            "When off, reset_controllers returns an error and MIDI handling follows the main branch."
        },
            { "reset_all_controllers.dsp.cc120.values", "Controller reset: DSP", "CC 120 All Sound Off", meta::SettingType::Choice, 2, 0, 3, receiveChoices, "", "COMPOSESIREN_RESETALLCONTROLLERS", "global", dspDescription },
            { "reset_all_controllers.dsp.cc121.values", "Controller reset: DSP", "CC 121 Reset All Controllers", meta::SettingType::Choice, 3, 0, 3, receiveChoices, "", "COMPOSESIREN_RESETALLCONTROLLERS", "global", dspDescription },
            { "reset_all_controllers.dsp.cc123.values", "Controller reset: DSP", "CC 123 All Notes Off", meta::SettingType::Choice, 2, 0, 3, receiveChoices, "", "COMPOSESIREN_RESETALLCONTROLLERS", "global", dspDescription },
            { "reset_all_controllers.bridge.cc120.values", "Controller reset: bridge", "CC 120 accepted values", meta::SettingType::Choice, 3, 0, 3, receiveChoices, "", "COMPOSESIREN_PARK_BRIDGE", "global", bridgeDescription },
            { "reset_all_controllers.bridge.cc121.values", "Controller reset: bridge", "CC 121 accepted values", meta::SettingType::Choice, 3, 0, 3, receiveChoices, "", "COMPOSESIREN_PARK_BRIDGE", "global", bridgeDescription },
            { "reset_all_controllers.bridge.cc123.values", "Controller reset: bridge", "CC 123 accepted values", meta::SettingType::Choice, 3, 0, 3, receiveChoices, "", "COMPOSESIREN_PARK_BRIDGE", "global", bridgeDescription },
            { "reset_all_controllers.bridge.cc120.output", "Controller reset: bridge", "CC 120 output value", meta::SettingType::Choice, 0, 0, 2, outputChoices, "", "COMPOSESIREN_PARK_BRIDGE", "global", outputDescription },
            { "reset_all_controllers.bridge.cc121.output", "Controller reset: bridge", "CC 121 output value", meta::SettingType::Choice, 0, 0, 2, outputChoices, "", "COMPOSESIREN_PARK_BRIDGE", "global", outputDescription },
            { "reset_all_controllers.bridge.cc123.output", "Controller reset: bridge", "CC 123 output value", meta::SettingType::Choice, 0, 0, 2, outputChoices, "", "COMPOSESIREN_PARK_BRIDGE", "global", outputDescription }
        }};
        return reset[static_cast<std::size_t>(id) - meta::settingCount];
    }
#endif
#if COMPOSESIREN_DSP_MIDI
    if (id == dspMidiInputDevice) {
        static constexpr meta::Setting row {
            "dsp_midi.input_device", "Mixer DSP MIDI", "MIDI input",
            meta::SettingType::String, 0.0, 0.0, 0.0, "", "",
            "COMPOSESIREN_DSP_MIDI", "global",
            "Extra MIDI-in for mixer DSP controls (reverb, pan, gain). "
            "Virtual port: the SirenOrchestra DSP destination (macOS/Linux). "
            "Any other entry: that MIDI input device as well."
        };
        return row;
    }
#endif
    return meta::settings[static_cast<std::size_t>(id)];
}

#if COMPOSESIREN_RESETALLCONTROLLERS
DspChannelModePolicy Settings::getDspChannelModePolicy() const
{
    return { getBool(controllerResetEnabled), {
        static_cast<MidiValueRule>(getInt(dspAllSoundsOffValues)),
        static_cast<MidiValueRule>(getInt(dspResetControllersValues)),
        static_cast<MidiValueRule>(getInt(dspAllNotesOffValues)) } };
}

BridgeChannelModePolicy Settings::getBridgeChannelModePolicy() const
{
    return { getBool(controllerResetEnabled), {
        static_cast<MidiValueRule>(getInt(bridgeAllSoundsOffValues)),
        static_cast<MidiValueRule>(getInt(bridgeResetControllersValues)),
        static_cast<MidiValueRule>(getInt(bridgeAllNotesOffValues)) }, {
        static_cast<MidiOutputValue>(getInt(bridgeAllSoundsOffOutput)),
        static_cast<MidiOutputValue>(getInt(bridgeResetControllersOutput)),
        static_cast<MidiOutputValue>(getInt(bridgeAllNotesOffOutput)) } };
}
#endif

bool Settings::isAvailable(Id id)
{
    const auto option = describe(id).requiredOption;
    if (option.empty()) return true;
    if (option == "COMPOSESIREN_SIREN_WAVES") return COMPOSESIREN_SIREN_WAVES != 0;
    if (option == "COMPOSESIREN_SETTINGS") return true;
    if (option == "COMPOSESIREN_MCP") return COMPOSESIREN_MCP != 0;
    if (option == "COMPOSESIREN_RECORD") return COMPOSESIREN_RECORD != 0;
    if (option == "COMPOSESIREN_PARK_BRIDGE") return COMPOSESIREN_PARK_BRIDGE != 0;
    if (option == "COMPOSESIREN_CLIC") return COMPOSESIREN_CLIC != 0;
    if (option == "COMPOSESIREN_DSP_MIDI") return COMPOSESIREN_DSP_MIDI != 0;
    if (option == "COMPOSESIREN_RUST_DSP") return COMPOSESIREN_RUST_DSP != 0;
#if COMPOSESIREN_RESETALLCONTROLLERS
    if (option == "COMPOSESIREN_RESETALLCONTROLLERS") return true;
#endif
    return false; // an option this build doesn't know about
}

juce::Identifier Settings::key(Id id)
{
    const auto name = describe(id).id;
    return juce::Identifier(juce::String(name.data(), name.size()));
}

// Clamped to the setting's range, as the type the property components expect.
juce::var Settings::normalised(Id id, const juce::var& v)
{
    const auto& d = describe(id);
    if (d.type == meta::SettingType::String)
        return v.toString();
    const double x = juce::jlimit(d.minimum, d.maximum, static_cast<double>(v));
    switch (d.type) {
        case meta::SettingType::Bool:   return x >= 0.5;
        case meta::SettingType::Int:
        case meta::SettingType::Choice: return static_cast<int>(std::lround(x));
        case meta::SettingType::Float:  return x;
        case meta::SettingType::String: break;
    }
    return x;
}

double Settings::get(Id id) const
{
    return static_cast<double>(tree.getProperty(key(id), describe(id).defaultValue));
}

juce::String Settings::getString(Id id) const
{
    return tree.getProperty(key(id), juce::String()).toString();
}

void Settings::set(Id id, double value)
{
    tree.setProperty(key(id), normalised(id, value), nullptr);
}

void Settings::setString(Id id, const juce::String& value)
{
    tree.setProperty(key(id), normalised(id, value), nullptr);
}

void Settings::resetToDefaults()
{
    for (std::size_t i = 0; i < settingCount; ++i) {
        const auto id = idAt(i);
        if (describe(id).type == meta::SettingType::String)
            setString(id, juce::String());
        else
            set(id, describe(id).defaultValue);
    }
}

juce::Value Settings::getValueObject(Id id)
{
    return tree.getPropertyAsValue(key(id), nullptr);
}

void Settings::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property)
{
    if (normalising) return;
    for (std::size_t i = 0; i < settingCount; ++i) {
        const auto id = idAt(i);
        if (key(id) != property) continue;

        // values typed in a property component arrive unclamped
        const auto current = tree.getProperty(property);
        const auto clean = normalised(id, current);
        if (clean != current || clean.isBool() != current.isBool()) {
            const juce::ScopedValueSetter<bool> guard(normalising, true);
            tree.setProperty(property, clean, nullptr);
        }
        if (describe(id).type == meta::SettingType::String)
            file->setValue(property.toString(), clean.toString());
        else
            file->setValue(property.toString(), static_cast<double>(clean));
#if COMPOSESIREN_RUST_DSP
        if (id == meta::SettingId::dspVariant || id == meta::SettingId::dspDataset || id == meta::SettingId::dspSkipSilentFrames)
            publishDspChoice(*this);
#endif
        listeners.call([id](Listener& l) { l.settingChanged(id); });
        return;
    }
}

} // namespace cs

namespace cs {

bool Settings::appliesTo(Id id, std::string_view variant)
{
    std::string_view rest = describe(id).appliesTo;
    if (rest.empty()) return true;
    for (;;) {
        const auto bar = rest.find('|');
        if (rest.substr(0, bar) == variant) return true;
        if (bar == std::string_view::npos) return false;
        rest.remove_prefix(bar + 1);
    }
}

std::string Settings::dspVariant() const
{
#if COMPOSESIREN_RUST_DSP
    return choiceAt(describe(Id::dspVariant), getInt(Id::dspVariant));
#else
    return "cpp";
#endif
}

} // namespace cs
