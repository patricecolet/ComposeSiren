//
// Created by joseph larralde on 20/02/2026.
//

#ifndef SIRENORCHESTRA_PLUGINEDITOR_H
#define SIRENORCHESTRA_PLUGINEDITOR_H

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class SirenOrchestraPluginEditor : public juce::AudioProcessorEditor
{
public:
    SirenOrchestraPluginEditor (SirenOrchestraPluginProcessor&);
    ~SirenOrchestraPluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
};


#endif //SIRENORCHESTRA_PLUGINEDITOR_H