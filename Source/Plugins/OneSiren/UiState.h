//
// Created by joseph larralde on 25/03/2026.
//

#ifndef COMPOSESIREN_ONESIREN_UISTATE_H
#define COMPOSESIREN_ONESIREN_UISTATE_H
#include <juce_data_structures/juce_data_structures.h>

// #include <lib/definitions/sirenProperties.h>
#include <uiParameterUtilities.h>

class UiState
{
public:
    UiState(juce::ValueTree tree) : state(tree) {
        jassert(state.isValid());
    }

    static juce::ValueTree createDefaultState() {
        juce::ValueTree t("UISTATE");
        t.setProperty("sirenCategory", "Alto", nullptr);
        t.setProperty("inputMidiChannel", 0, nullptr); // 0 is Any
        t.setProperty("outputMidiChannel", 0, nullptr); // 0 is Thru
        return t;
    }

    sirenCategory getSirenCategory() const {
        const juce::String c = state.getProperty("sirenCategory");
        return sirenPropertiesByName.at(c.toStdString())->category;
    }
    void setSirenCategory(sirenCategory c) {
        const juce::String name = sirenCategoriesData.at(c).name;
        state.setProperty("sirenCategory", name, nullptr);
    }
    AnyOrOneBasedMidiChannel getInputMidiChannel() const {
        return getChannelProperty("inputMidiChannel");
    }
    void setInputMidiChannel(const AnyOrOneBasedMidiChannel& c) {
        setChannelProperty("inputMidiChannel", c);
    }
    AnyOrOneBasedMidiChannel getOutputMidiChannel() const {
        return getChannelProperty("outputMidiChannel");
    }
    void setOutputMidiChannel(const AnyOrOneBasedMidiChannel& c) {
        setChannelProperty("outputMidiChannel", c);
    }

private:
    juce::ValueTree state;

    AnyOrOneBasedMidiChannel getChannelProperty(const juce::Identifier& name) const {
        const int c = state.getProperty(name);
        return midiChannelByMenuIndex.at(c);
    }
    void setChannelProperty(const juce::Identifier& name,
                            const AnyOrOneBasedMidiChannel& c) {
        state.setProperty(
            name,
            menuIndexByMidiChannel.at(c),
            nullptr
        );
    }
};

#endif //COMPOSESIREN_ONESIREN_UISTATE_H
