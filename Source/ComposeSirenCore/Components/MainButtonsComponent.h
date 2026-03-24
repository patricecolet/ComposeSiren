//
// Created by joseph larralde on 21/03/2026.
//

#ifndef COMPOSESIREN_RESETCOMPONENT_H
#define COMPOSESIREN_RESETCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>

class ResetComponent : public juce::Component
{
    juce::TextButton resetButton;
    juce::TextButton selectResourcesButton;

public:
    ResetComponent()
    {
        resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        // resetButton.setColour(juce::TextButton::buttonOnColourId , juce::Colours::grey);
        resetButton.setColour(juce::TextButton::textColourOffId , juce::Colours::black);
        resetButton.setButtonText ("Reset");
        addAndMakeVisible(resetButton);

        selectResourcesButton.setColour(juce::TextButton::buttonColourId, juce::Colours::whitesmoke);
        selectResourcesButton.setColour(juce::TextButton::textColourOffId , juce::Colours::black);
        selectResourcesButton.setButtonText("Set resources directory");
        addAndMakeVisible(selectResourcesButton);
    }

    ~ResetComponent() = default;

    void paint(juce::Graphics& g) override
    {

    }

    void resized() override
    {

    }
};


#endif //COMPOSESIREN_RESETCOMPONENT_H