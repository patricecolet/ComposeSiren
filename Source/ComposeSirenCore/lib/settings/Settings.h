#pragma once

// Power-user settings of the plugins. What exists, with its type, range,
// default and description, comes from lib/definitions/generated/
// SettingsMetadata.h, committed, generated from mecaviv-dev-root's
// resources/metadata/composesiren-settings.csv by `franz doctor fix meta`.
// The compile-gated controller-reset rows extend it in Settings::describe().
//
// The values are stored per user (a properties file next to the other
// Mecanique Vivante settings) and the Settings... menu item edits them, in
// every build; a setting whose option this build lacks keeps its default and
// isn't shown (isAvailable). Message
// thread only. Share one instance per process with
// juce::SharedResourcePointer<cs::Settings>.

#include <juce_data_structures/juce_data_structures.h>
#include "../definitions/generated/SettingsMetadata.h"
#include "../dsp/ChannelModePolicy.h"

namespace cs {

class Settings : private juce::ValueTree::Listener
{
public:
    using Id = mecaviv::metadata::SettingId;

#if COMPOSESIREN_RESETALLCONTROLLERS
    // Optional extension: leave the generated base table and disabled builds
    // unchanged. Stored by name, like the generated settings.
    static constexpr Id controllerResetEnabled = static_cast<Id>(mecaviv::metadata::settingCount);
    static constexpr Id dspAllSoundsOffValues = static_cast<Id>(mecaviv::metadata::settingCount + 1);
    static constexpr Id dspResetControllersValues = static_cast<Id>(mecaviv::metadata::settingCount + 2);
    static constexpr Id dspAllNotesOffValues = static_cast<Id>(mecaviv::metadata::settingCount + 3);
    static constexpr Id bridgeAllSoundsOffValues = static_cast<Id>(mecaviv::metadata::settingCount + 4);
    static constexpr Id bridgeResetControllersValues = static_cast<Id>(mecaviv::metadata::settingCount + 5);
    static constexpr Id bridgeAllNotesOffValues = static_cast<Id>(mecaviv::metadata::settingCount + 6);
    static constexpr Id bridgeAllSoundsOffOutput = static_cast<Id>(mecaviv::metadata::settingCount + 7);
    static constexpr Id bridgeResetControllersOutput = static_cast<Id>(mecaviv::metadata::settingCount + 8);
    static constexpr Id bridgeAllNotesOffOutput = static_cast<Id>(mecaviv::metadata::settingCount + 9);
    static constexpr std::size_t settingCount = mecaviv::metadata::settingCount + 10;
    static bool isControllerResetSetting(Id id) {
        return static_cast<std::size_t>(id) >= mecaviv::metadata::settingCount
            && static_cast<std::size_t>(id) < settingCount;
    }
    DspChannelModePolicy getDspChannelModePolicy() const;
    BridgeChannelModePolicy getBridgeChannelModePolicy() const;
#else
    static constexpr std::size_t settingCount = mecaviv::metadata::settingCount;
#endif

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void settingChanged(Id) = 0;
    };

    Settings();
    ~Settings() override;

    static const mecaviv::metadata::Setting& describe(Id id);
    static Id idAt(std::size_t index) { return static_cast<Id>(index); }
    // false when the CMake option the setting needs is not compiled in
    static bool isAvailable(Id id);

    double get(Id id) const;
    bool getBool(Id id) const { return get(id) >= 0.5; }
    int getInt(Id id) const { return static_cast<int>(std::lround(get(id))); }
    float getFloat(Id id) const { return static_cast<float>(get(id)); }
    /// String settings (SettingType::String); empty for the others.
    juce::String getString(Id id) const;

    void set(Id id, double value);
    void setString(Id id, const juce::String& value);
    void resetToDefaults();

    // the stored value, for juce property components
    juce::Value getValueObject(Id id);

    void addListener(Listener* l) { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

private:
    static juce::Identifier key(Id id);
    static juce::var normalised(Id id, const juce::var& v);
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override;

    juce::ValueTree tree { "ComposeSirenSettings" };
    juce::ListenerList<Listener> listeners;
    bool normalising = false;
    std::unique_ptr<juce::PropertiesFile> file;
};

} // namespace cs
