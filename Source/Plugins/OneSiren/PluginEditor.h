//
// Created by joseph larralde on 20/02/2026.
//

#ifndef ONESIREN_PLUGINEDITOR_H
#define ONESIREN_PLUGINEDITOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <Components/MainButtonsComponent.h>
#include <Components/VoiceManagerComponent.h>
#include <Components/SirenStripComponent.h>
#include <Components/DbRangesMidiKeyboardComponent.h>
#include "PluginProcessor.h"

class OneSirenPluginEditor : public juce::AudioProcessorEditor,
                             public VoiceManagerState::Listener
                             // public MainButtonsComponent::Listener
{
public:
    OneSirenPluginEditor(OneSirenPluginProcessor&);
    ~OneSirenPluginEditor() override;

    //==========================================================================
    // AudioProcessorEditor
    void paint(juce::Graphics&) override;
    void resized() override;

    //==========================================================================
    // MainButtonsComponent::Listener
    // void resetSiren() override;
    // std::string getResourcesPath() override;
    // void selectedNewResourcesPath(const std::string&) override;

    //==========================================================================
    // VoiceManagerState::Listener
    void categoryChanged(sirenCategory) override;

private:
    OneSirenPluginProcessor& audioProcessor;
    MainButtonsComponent mainButtons;
    VoiceManagerComponent voiceManager;
    SirenStripComponent sirenStrip;
    DbRangesMidiKeyboardComponent midiKeyboard;
};

#endif //ONESIREN_PLUGINEDITOR_H
