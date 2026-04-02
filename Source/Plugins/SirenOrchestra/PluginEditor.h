//
// Created by joseph larralde on 20/02/2026.
//

#ifndef SIRENORCHESTRA_PLUGINEDITOR_H
#define SIRENORCHESTRA_PLUGINEDITOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <Components/MainButtonsComponent.h>
#include <Components/VoiceManagerComponent.h>
#include <Components/SirenStripComponent.h>
#include <Components/DbRangesMidiKeyboardComponent.h>
#include <Components/ReverbStripComponent.h>
#include "PluginProcessor.h"

class SirenOrchestraPluginEditor : public juce::AudioProcessorEditor,
                                   public VoiceManagerState::Listener
{
public:
    SirenOrchestraPluginEditor(SirenOrchestraPluginProcessor&);
    ~SirenOrchestraPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::unique_ptr<juce::Drawable> wood;

    const std::vector<sirenId> sirenOrder{ S3, S4, S1, S2, S5, S6, S7 };

    SirenOrchestraPluginProcessor& audioProcessor;
    // MainButtonsComponent mainButtons;
    VoiceManagerComponent voiceManager;
    std::map<sirenId, std::unique_ptr<SirenStripComponent>> sirenStrips;
    // std::vector<std::unique_ptr<SirenStripComponent>> sirenStripComponents;
    ReverbStripComponent rvbStrip;
    DbRangesMidiKeyboardComponent midiKeyboard;
};


#endif //SIRENORCHESTRA_PLUGINEDITOR_H