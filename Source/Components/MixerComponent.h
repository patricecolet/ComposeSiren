//
// Created by joseph larralde on 24/01/2026.
//

#ifndef COMPOSESIREN_MIXERCOMPONENT_H
#define COMPOSESIREN_MIXERCOMPONENT_H

#include <JuceHeader.h>
#include "MixerStripComponent.h"
#include "ReverbComponent.h"
#include "../PluginProcessor.h"

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



#endif //COMPOSESIREN_MIXERCOMPONENT_H