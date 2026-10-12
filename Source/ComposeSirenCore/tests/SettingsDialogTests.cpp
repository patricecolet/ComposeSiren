// The Settings... window is in every build: it opens, with a row for each setting this
// build has (an option gates only its own rows), or a note when there is none.
// Usage: ComposeSirenSettingsDialogTests

#include "lib/settings/SettingsDialog.h"

#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    int available = 0, reset = 0;
    for (std::size_t i = 0; i < cs::Settings::settingCount; ++i) {
        const auto id = cs::Settings::idAt(i);
        if (!cs::Settings::isAvailable(id)) continue;
        ++available;
        if (cs::Settings::describe(id).requiredOption == "COMPOSESIREN_RESETALLCONTROLLERS") ++reset;
    }
    int failures = 0;
    auto check = [&](bool ok, const char* what) {
        std::cout << (ok ? "ok   " : "FAIL ") << what << "\n";
        if (!ok) ++failures;
    };
    check((reset > 0) == (COMPOSESIREN_RESETALLCONTROLLERS != 0),
          "the controller reset rows come with COMPOSESIREN_RESETALLCONTROLLERS only");
    check((available > 0) == (COMPOSESIREN_RESETALLCONTROLLERS || COMPOSESIREN_RUST_DSP
                              || COMPOSESIREN_SIREN_WAVES || COMPOSESIREN_CLIC),
          "rows only for the options of this build");
    {
        cs::SettingsDialog dialog;
        check(dialog.getWidth() > 0 && dialog.getHeight() > 0, "the window opens");
        check(dialog.getNumChildComponents() >= 2, "rows or the empty note, and Reset to defaults");
    }
    std::cout << available << " setting(s) in this build, " << reset << " controller reset\n";
    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
