/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"



//==============================================================================
// class headComponent : head of the main window
class headComponent   : public juce::Component
{
public:
    headComponent();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Label labelPluginTitle;
    juce::Label labelPluginSubTitle;
    juce::Image imgLogo;
};

//==============================================================================


//==============================================================================
// class MainCommandsComponent : main commands
class MainCommandsComponent   : public juce::Component
{
public:
    MainCommandsComponent(SirenePlugAudioProcessor&);
    ~MainCommandsComponent();

    void paint (juce::Graphics&) override;
    void resized() override;
    
    juce::TextButton resetButton;

private:
    SirenePlugAudioProcessor& audioProcessor;
    
   
    
};

//==============================================================================

//==============================================================================
// Strip de mixage individuelle pour une sirène
class MixerStripComponent : public juce::Component, 
                           public juce::Slider::Listener,
                           private juce::Timer
{
public:
    MixerStripComponent(SirenePlugAudioProcessor& p, int sireneNum);
    ~MixerStripComponent();

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    SirenePlugAudioProcessor& audioProcessor;
    int sireneNumber;
    
    juce::Label nameLabel;
    juce::Slider panKnob;
    juce::Label panLabel;
    juce::Label masterVolumeLabel; // Pour le master volume CC70
    juce::Slider masterVolumeSlider; // Volume indépendant CC70
    
    // LED Note On/Off
    bool ledState = false;
    
    void timerCallback();
};

//==============================================================================
// Section de reverb
class ReverbComponent : public juce::Component,
                       public juce::Slider::Listener,
                       public juce::Button::Listener
{
public:
    ReverbComponent(SirenePlugAudioProcessor& p);
    ~ReverbComponent();

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
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
    juce::Label ccInfoLabel; // Pour afficher les CC sur canal 16
};

//==============================================================================
// Composant mixeur complet
class MixerComponent : public juce::Component
{
public:
    MixerComponent(SirenePlugAudioProcessor& p);
    ~MixerComponent();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SirenePlugAudioProcessor& audioProcessor;
    
    std::unique_ptr<MixerStripComponent> strips[7];
    std::unique_ptr<ReverbComponent> reverb;
};

//==============================================================================


class SirenePlugAudioProcessorEditor  : public juce::AudioProcessorEditor//, public juce::Button::Listener

{
public:
    SirenePlugAudioProcessorEditor (SirenePlugAudioProcessor&);
    ~SirenePlugAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    //==============================================================================



private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SirenePlugAudioProcessor& audioProcessor;
    
    headComponent head;
    MainCommandsComponent mainCommands;
    MixerComponent mixer;
    
    /*
    void buttonClicked (juce::Button* button) override
    {
        //juce::String buttonName = button->getComponentID();
        //if (buttonName == "reset")
        if (button == &mainCommands.resetButton)
        {
            std::cout << "Reset"<<std::endl;
            audioProcessor.myMidiInHandler -> resetSireneCh(1);
            audioProcessor.myMidiInHandler -> resetSireneCh(2);
            audioProcessor.myMidiInHandler -> resetSireneCh(3);
            audioProcessor.myMidiInHandler -> resetSireneCh(4);
            audioProcessor.myMidiInHandler -> resetSireneCh(5);
            audioProcessor.myMidiInHandler -> resetSireneCh(6);
            audioProcessor.myMidiInHandler -> resetSireneCh(7);

        }
    }
     */

    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SirenePlugAudioProcessorEditor)
};
