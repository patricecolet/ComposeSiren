//
// Created by joseph larralde on 24/01/2026.
//

#ifndef COMPOSESIREN_MIXERSTRIP_H
#define COMPOSESIREN_MIXERSTRIP_H

#include <juce_gui_basics/juce_gui_basics.h>

// #include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
// Strip de mixage individuelle pour une sirène
class MixerStripComponent : public juce::Component,
                            public juce::Slider::Listener,
                            private juce::Timer
{
public:
    MixerStripComponent(SirenePlugAudioProcessor& p, int sireneNum);
    ~MixerStripComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    SirenePlugAudioProcessor& audioProcessor;
    int sireneNumber;

    juce::Label nameLabel;
    juce::Slider panKnob;
    juce::Label panLabel;
    juce::Label volumeLabel; // Pour le master volume CC70
    juce::Slider volumeSlider; // Volume indépendant CC70

    // LED Note On/Off
    bool ledState = false;

    void timerCallback();
};


#endif //COMPOSESIREN_MIXERSTRIP_H