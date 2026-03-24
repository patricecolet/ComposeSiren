//
// Created by joseph larralde on 24/01/2026.
//

#ifndef COMPOSESIREN_REVERBCOMPONENT_H
#define COMPOSESIREN_REVERBCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
// #include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
// Section de reverb
class ReverbComponent : public juce::Component,
                       public juce::Slider::Listener,
                       public juce::Button::Listener,
                       private juce::Timer
{
public:
    ReverbComponent(SirenePlugAudioProcessor& p);
    ~ReverbComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    void timerCallback() override;
    SirenePlugAudioProcessor& audioProcessor;

    juce::ToggleButton enableButton;
    juce::Slider roomSizeSlider;
    juce::Slider wetSlider;
    juce::Slider dampSlider;
    juce::Slider widthSlider;
    juce::Slider highpassSlider;
    juce::Slider lowpassSlider;

    juce::Label titleLabel;
    juce::Label roomSizeLabel;
    juce::Label wetLabel;
    juce::Label dampLabel;
    juce::Label widthLabel;
    juce::Label highpassLabel;
    juce::Label lowpassLabel;
};

#endif //COMPOSESIREN_REVERBCOMPONENT_H