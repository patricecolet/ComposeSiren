/*
  ==============================================================================

    PluginProcessor_S1.h
    ComposeSiren S1 Alto - Solo Plugin
    
    Plugin simplifié contenant uniquement la sirène S1 (Alto)
    Sans mixeur, pan, limiter ou reverb - le DAW gère ces aspects

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CS_midiIN.h"

//==============================================================================
class SireneS1AudioProcessor  : public juce::AudioProcessor, private juce::Timer
{
public:
    //==============================================================================
    SireneS1AudioProcessor();
    ~SireneS1AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // Accès aux données MIDI pour l'affichage dans l'interface
    struct MidiState {
        bool noteOn = false;
        int currentNote = 0;
        int currentVelocity = 0;
        std::map<int, int> activeCC;  // CC number -> value
    };
    
    MidiState getMidiState() const { return midiState; }
    
    // Accès public pour l'interface
    MidiIn* myMidiInHandler;
    Synth* mySynth;

private:
    //==============================================================================
    void timerCallback() override;
    
    int sampleCountForMidiInTimer = 0;
    MidiState midiState;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SireneS1AudioProcessor)
};

