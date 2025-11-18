/*
  ==============================================================================

    SoloPluginEditor.h
    ComposeSiren Solo Plugin (modèle paramétrable)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SoloPluginProcessor.h"

//==============================================================================
class SoloPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Timer,
                                        private juce::Slider::Listener
{
public:
    SoloPluginAudioProcessorEditor (SoloPluginAudioProcessor&);
    ~SoloPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    void timerCallback() override;
    void createKnob(juce::Slider& slider, juce::Label& label, const juce::String& name, int ccNumber);
    juce::String formatMidiNote (int midiNote) const;
    juce::String buildAmbitusText() const;
    
    SoloPluginAudioProcessor& audioProcessor;
    
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
    juce::Label ambitusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoloPluginAudioProcessorEditor)
};

