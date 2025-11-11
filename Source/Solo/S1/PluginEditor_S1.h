/*
  ==============================================================================

    PluginEditor_S1.h
    ComposeSiren S1 Alto - Interface utilisateur Solo
    
    Interface minimaliste affichant l'état MIDI en temps réel

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class SireneS1AudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Timer,
                                       private juce::Slider::Listener
{
public:
    SireneS1AudioProcessorEditor (SireneS1AudioProcessor&);
    ~SireneS1AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    void timerCallback() override;
    void createKnob(juce::Slider& slider, juce::Label& label, const juce::String& name, int ccNumber);
    
    SireneS1AudioProcessor& audioProcessor;
    
    // État MIDI pour affichage
    bool lastNoteOn;
    int lastNote;
    int lastVelocity;
    std::map<int, int> lastCC;
    
    // Sections de l'interface
    juce::GroupComponent vibratoGroup;
    juce::GroupComponent tremoloGroup;
    juce::GroupComponent envelopeGroup;
    juce::GroupComponent portamentoGroup;
    
    // Knobs Vibrato
    juce::Slider vibratoDepthKnob;      // CC1
    juce::Slider vibratoRateKnob;       // CC9
    juce::Slider vibratoAttackKnob;     // CC11
    juce::Label vibratoDepthLabel;
    juce::Label vibratoRateLabel;
    juce::Label vibratoAttackLabel;
    
    // Knobs Tremolo
    juce::Slider tremoloRateKnob;       // CC15
    juce::Slider tremoloDepthKnob;      // CC92
    juce::Label tremoloRateLabel;
    juce::Label tremoloDepthLabel;
    
    // Knobs Enveloppe
    juce::Slider attackKnob;            // CC73
    juce::Slider releaseKnob;           // CC72
    juce::Label attackLabel;
    juce::Label releaseLabel;
    
    // Knobs Portamento
    juce::Slider portamentoKnob;        // CC5
    juce::Label portamentoLabel;
    
    // Indicateur MIDI (en haut)
    juce::Label noteLabel;
    juce::Label velocityLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SireneS1AudioProcessorEditor)
};

