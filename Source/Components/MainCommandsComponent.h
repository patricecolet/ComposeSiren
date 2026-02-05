//
// Created by joseph larralde on 25/01/2026.
//

#ifndef COMPOSESIREN_MAINCOMMANDSCOMPONENT_H
#define COMPOSESIREN_MAINCOMMANDSCOMPONENT_H

#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
// class MainCommandsComponent : main commands
class MainCommandsComponent   : public juce::Component,
                                 public juce::Slider::Listener,
                                 //public juce::Button::Listener,
                                 private juce::Timer
{
public:
    MainCommandsComponent(SirenePlugAudioProcessor&);
    ~MainCommandsComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    // void buttonClicked(juce::Button* button) override;

    juce::TextButton resetButton;

private:
    void timerCallback() override;
    SirenePlugAudioProcessor& audioProcessor;

    // Contrôles globaux (Canal 16)
    juce::Label masterVolumeLabel;
    juce::Slider masterVolumeSlider; // CC7 canal 16
};

#endif //COMPOSESIREN_MAINCOMMANDSCOMPONENT_H