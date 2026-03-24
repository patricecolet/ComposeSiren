/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#ifndef COMPOSESIREN_PLUGINEDITOR_H
#define COMPOSESIREN_PLUGINEDITOR_H

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components/MainCommandsComponent.h"
#include "Components/MixerComponent.h"

//==============================================================================
// class headComponent : head of the main window
class HeadComponent : public juce::Component
{
public:
    HeadComponent();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Label labelPluginTitle;
    Label labelPluginSubTitle;
    Image imgLogo;
};
//=============================================================================

class SirenePlugAudioProcessorEditor :  public AudioProcessorEditor//,
                                        // public Button::Listener,
                                        // public MainCommandsComponent::Listener,
                                        // public MixerStripComponent::Listener
{
public:
    SirenePlugAudioProcessorEditor (SirenePlugAudioProcessor&);
    ~SirenePlugAudioProcessorEditor() override;

    //=========================================================================
    // AudioProcessorEditor
    void paint (juce::Graphics&) override;
    void resized() override;
    //=========================================================================
    // MainCommandsComponent::Listener
    // void onReset() override;
    // void onShowResourcesDirectory() override;
    // void onMasterVolumeSliderValueChanged(float v) override;
    //=========================================================================
    // MixerStripComponent::Listener
    // void onMixerStripVolumeSliderValueChanged(int id, float v) override;
    // void onMixerStripPanSliderValueChanged(int id, float v) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SirenePlugAudioProcessor& audioProcessor;
    
    HeadComponent head;
    MainCommandsComponent mainCommands;
    MixerComponent mixer;
    // std::vector<std::unique_ptr<SirenStripComponent>> orchestra;

    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SirenePlugAudioProcessorEditor)
};

#endif // COMPOSESIREN_PLUGINEDITOR_H