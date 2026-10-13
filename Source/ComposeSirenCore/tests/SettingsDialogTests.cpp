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
#if COMPOSESIREN_RUST_DSP
        // per dsp.variant: a row is greyed out exactly when its applies_to leaves the variant out
        using Id = cs::Settings::Id;
        const auto& vd = cs::Settings::describe(Id::dspVariant);
        juce::StringArray variants;
        variants.addTokens(juce::String(vd.choices.data(), vd.choices.size()), "|", {});
        bool matches = true, datasetRust = true, skipLoops = true;
        for (const auto& v : variants) {
            const auto name = v.toStdString();
            dialog.showApplicable(name);
            for (std::size_t i = 0; i < cs::Settings::settingCount; ++i) {
                const auto id = cs::Settings::idAt(i);
                if (cs::Settings::isAvailable(id) && dialog.isRowEnabled(id) != cs::Settings::appliesTo(id, name))
                    matches = false;
            }
            datasetRust = datasetRust && dialog.isRowEnabled(Id::dspDataset) == (name != "cpp");
            const bool loops = name == "frames" || v.startsWith("trio");
            skipLoops = skipLoops && dialog.isRowEnabled(Id::dspSkipSilentFrames) == loops;
            std::cout << "     " << name << ": data set " << (dialog.isRowEnabled(Id::dspDataset) ? "on" : "greyed")
                      << ", skip silent frames " << (dialog.isRowEnabled(Id::dspSkipSilentFrames) ? "on" : "greyed")
                      << ", DSP row " << (dialog.isRowEnabled(Id::dspVariant) ? "on" : "greyed") << "\n";
        }
        check(variants.size() > 1 && matches, "every row follows its applies_to, for every variant");
        check(datasetRust, "dsp.dataset greyed out for cpp only");
        check(skipLoops, "dsp.skip_silent_frames only for the variants that loop over frames");
#endif
    }
    std::cout << available << " setting(s) in this build, " << reset << " controller reset\n";
    std::cout << (failures ? "FAILED: " : "ok: ") << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
