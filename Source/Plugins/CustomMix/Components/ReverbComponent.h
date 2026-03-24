//
// Created by joseph larralde on 24/01/2026.
//

#ifndef COMPOSESIREN_REVERBCOMPONENT_H
#define COMPOSESIREN_REVERBCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
// #include <JuceHeader.h>
// #include "../PluginProcessor.h"

//==============================================================================
// Section de reverb
class ReverbComponent : public juce::Component,
                       public juce::Slider::Listener,
                       public juce::Button::Listener,
                       private juce::Timer
{
public:
    // class Listener {
    // public:
    //     virtual ~Listener() = default;
    //     virtual void onReverbRoomsizeSliderValueChanged(float v) = 0;
    //     virtual void onReverbDrywetSliderValueChanged(float v) = 0;
    //     virtual void onReverbDampingSliderValueChanged(float v) = 0;
    //     virtual void onReverbWidthSliderValueChanged(float v) = 0;
    //     virtual void onReverbHighpassSliderValueChanged(float v) = 0;
    //     virtual void onReverbLowpassSliderValueChanged(float v) = 0;
    // };

    ReverbComponent(/*SirenePlugAudioProcessor& p*/Listener* l);
    ~ReverbComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    void timerCallback() override;
    // SirenePlugAudioProcessor& audioProcessor;

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

    Listener* listener;
};

#endif //COMPOSESIREN_REVERBCOMPONENT_H