#pragma once

// The Settings... window, in every build. Built from the settings
// metadata: one section per group, one row per setting this build has, with
// the description as tooltip; rows the selected dsp.variant doesn't use are greyed out. A stand-in until the settings GUI is decided.

#include <juce_gui_basics/juce_gui_basics.h>
#include "Settings.h"

namespace cs {

class SettingsDialog : public juce::Component,
                       private Settings::Listener
{
public:
    SettingsDialog();
    ~SettingsDialog() override;

    // Greys out the rows the `dsp.variant` named `variant` doesn't use (Settings::appliesTo),
    // the others enabled. The window does it for the selected variant, and again on a change.
    void showApplicable(std::string_view variant);
    // false when the row is greyed out; true for a setting without a row
    bool isRowEnabled(Settings::Id id) const;

    // Opens the window (non-modal) near `parent`. It deletes itself when closed.
    static void show(juce::Component* parent);

    void resized() override;

private:
    juce::SharedResourcePointer<Settings> settings;
    juce::PropertyPanel panel;
    juce::TextButton reset { "Reset to defaults" };
    juce::Label empty; // shown when this build has no setting
    juce::TooltipWindow tooltips { this };
    std::vector<std::pair<Settings::Id, juce::PropertyComponent*>> rowsById; // owned by the panel
    void settingChanged(Settings::Id id) override;
};

} // namespace cs

