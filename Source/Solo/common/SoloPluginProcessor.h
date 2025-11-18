/*
  ==============================================================================

    SoloPluginProcessor.h
    ComposeSiren Solo Plugin (modèle paramétrable)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <map>
#include "CS_midiIN.h"
#include "synth.h"

#ifndef SIREN_MODEL
    #define SIREN_MODEL "S1"
#endif

#ifndef SIREN_DISPLAY_NAME
    #define SIREN_DISPLAY_NAME "S1 Alto"
#endif

namespace SoloConfig
{
    inline juce::String getModel()
    {
        return juce::String(SIREN_MODEL);
    }

    inline juce::String getDisplayName()
    {
        return juce::String(SIREN_DISPLAY_NAME);
    }

    inline int getMidiChannel()
    {
        auto model = getModel().toUpperCase();
        if (model.startsWith("S"))
        {
            auto channelString = model.substring(1).trim();
            int channel = channelString.getIntValue();
            if (channel >= 1 && channel <= 16)
                return channel;
        }
        return 1;
    }

    inline void getAmbitus(int& noteMin, int& noteMax)
    {
        auto model = getModel().toUpperCase();
        if (model == "S3")
        {
            noteMin = 24; noteMax = 64;
        }
        else if (model == "S4")
        {
            noteMin = 24; noteMax = 65;
        }
        else if (model == "S5" || model == "S6")
        {
            noteMin = 36; noteMax = 79;
        }
        else if (model == "S7")
        {
            noteMin = 36; noteMax = 79;
        }
        else // S1 et défaut
        {
            noteMin = 24; noteMax = 72;
        }
    }
}

//==============================================================================
class SoloPluginAudioProcessor  : public juce::AudioProcessor, private juce::Timer
{
public:
    //==============================================================================
    SoloPluginAudioProcessor();
    ~SoloPluginAudioProcessor() override;

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
    struct MidiState {
        bool noteOn = false;
        int currentNote = 0;
        int currentVelocity = 0;
        std::map<int, int> activeCC;  // CC number -> value
    };
    
    MidiState getMidiState() const { return midiState; }
    MidiIn* getMidiHandler() const { return myMidiInHandler; }
    int getSoloChannel() const { return soloMidiChannel; }
    const juce::String& getModelName() const { return sirenModel; }
    
private:
    //==============================================================================
    void timerCallback() override;
    Sirene* resolveActiveSirene(int channel) const;
    int* getIntFromMidiMessage(const void* data, int size);
    
    int sampleCountForMidiInTimer = 0;
    int* midiMessageIntArray = nullptr;
    MidiState midiState;
    MidiIn* myMidiInHandler = nullptr;
    Synth* mySynth = nullptr;
    Sirene* activeSirene = nullptr;
    const int soloMidiChannel;
    juce::String sirenModel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoloPluginAudioProcessor)
};

