/*
  ==============================================================================

    PluginEditor_S1.h
    ComposeSiren S1 Alto - Interface utilisateur Solo
    
    Interface minimaliste affichant l'état MIDI en temps réel

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor_S1.h"

//==============================================================================
class SireneS1AudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    SireneS1AudioProcessorEditor (SireneS1AudioProcessor&);
    ~SireneS1AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
    SireneS1AudioProcessor& audioProcessor;
    
    // État MIDI pour affichage
    bool lastNoteOn;
    int lastNote;
    int lastVelocity;
    std::map<int, int> lastCC;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SireneS1AudioProcessorEditor)
};

