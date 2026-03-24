//
// Created by joseph larralde on 25/01/2026.
//

#ifndef COMPOSESIREN_MAINCOMMANDSCOMPONENT_H
#define COMPOSESIREN_MAINCOMMANDSCOMPONENT_H

// #include <juce_core/juce_core.h>
// already includes juce_core which includes juce_graphics
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// #include <JuceHeader.h>
// #include "../PluginProcessor.h"

//==============================================================================
// class MainCommandsComponent : main commands
class MainCommandsComponent :   public juce::Component,
                                public juce::Slider::Listener,
                                public juce::Button::Listener,
                                private juce::Timer
{
public:
    // class Listener {
    // public:
    //     virtual ~Listener() = default;
    //     virtual void onReset() = 0;
    //     virtual void onShowResourcesDirectory() = 0;
    //     virtual void onMasterVolumeSliderValueChanged(float v) = 0;
    // };

    // MainCommandsComponent(/*SirenePlugAudioProcessor&*/);
    MainCommandsComponent(juce::AudioProcessorValueTreeState&);
    ~MainCommandsComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

    // should be set from PluginEditor's timerCallback
    void setMasterVolumeSliderValue();

private:
    void timerCallback() override;
    // SirenePlugAudioProcessor& audioProcessor;

    juce::TextButton resetButton;
    juce::TextButton showResourcesButton;

    // Contrôles globaux (Canal 16)
    juce::Label masterVolumeLabel;
    juce::Slider masterVolumeSlider; // CC7 canal 16

    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::AudioProcessorValueTreeState& apvts;
};

#endif //COMPOSESIREN_MAINCOMMANDSCOMPONENT_H