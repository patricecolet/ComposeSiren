//
// Created by joseph larralde on 24/01/2026.
//

#include "../../../ComposeSirenCore/Components/MixerComponent.h"

//==============================================================================
// MixerComponent - Mixeur complet
MixerComponent::MixerComponent(/*SirenePlugAudioProcessor& p*/MixerStripComponent::Listener* msl, ReverbComponent::Listener* rl)
    // : audioProcessor(p)
{
    // Créer les 7 strips
    for (int i = 0; i < 7; i++)
    {
        strips[i] = std::make_unique<MixerStripComponent>(/*audioProcessor,*/msl, i + 1);
        addAndMakeVisible(strips[i].get());
    }

    // Créer la section reverb
    reverb = std::make_unique<ReverbComponent>(/*audioProcessor*/rl);
    addAndMakeVisible(reverb.get());
}

MixerComponent::~MixerComponent()
{
}

void MixerComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(30, 30, 30));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);
}

void MixerComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    int stripWidth = 70; // Réduit - un seul fader maintenant
    int reverbWidth = 210; // Pour 6 knobs

    // Positionner les 7 strips
    for (int i = 0; i < 7; i++)
    {
        strips[i]->setBounds(area.removeFromLeft(stripWidth));
        area.removeFromLeft(3); // Espacement
    }

    // Positionner la reverb
    reverb->setBounds(area);
}
