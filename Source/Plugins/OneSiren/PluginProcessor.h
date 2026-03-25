//
// Created by joseph larralde on 20/02/2026.
//

#ifndef ONESIREN_PLUGINPROCESSOR_H
#define ONESIREN_PLUGINPROCESSOR_H

#include <JuceHeader.h>
#include <MidiRouter.h>
#include <MidiScheduler.h>
#include <apvtsUtilities.h>
#include <lib/wrappers/SirenVoice.h>
#include <Components/VoiceManagerState.h>

#include "Components/MainButtonsComponent.h"

class OneSirenPluginProcessor :
    public juce::AudioProcessor,
    // public juce::Timer,
    public VoiceManagerState::Listener,
    public MainButtonsComponent::Listener
{
public:
    //==========================================================================
    OneSirenPluginProcessor();
    ~OneSirenPluginProcessor() override;

    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    // MidiKeyboardState::Listener callbacks (called from UI thread)
    // must write note events atomically to be sent from processBlock
    //--------------------------------------------------------------------------
    // void handleNoteOn(juce::MidiKeyboardState* source,
    //                   int channel, int noteNumber, float velocity) override;
    // void handleNoteOff(juce::MidiKeyboardState* source,
    //                    int channel, int noteNumber, float velocity) override;

    // AudioProcessorValueTreeState::Listener callback (called from UI thread)
    // writes cc events atomically to be sent from processBlock
    //--------------------------------------------------------------------------
    // void parameterChanged(const juce::String& parameterID,
    //                       float newValue) override;

    // MainButtonsComponents::Listener callbacks
    //--------------------------------------------------------------------------
    void resetSiren() override;
    std::string getResourcesPath() override;
    void selectedNewResourcesPath(const std::string&) override;

    // VoiceManagerState::Listener callbacks
    //--------------------------------------------------------------------------
    void categoryChanged(sirenCategory newCategory) override;
    void midiInputChanged(AnyOrOneBasedMidiChannel) override;
    void midiOutputChanged(AnyOrOneBasedMidiChannel) override;

    // void timerCallback() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==========================================================================
    const juce::String getName() const override;

    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    // this might be called by the host (e.g. insted of all note off ?)
    // void reset() override;

    //==========================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==========================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    //==========================================================================

    juce::AudioProcessorValueTreeState& getAudioProcessorValueTreeState();
    juce::MidiKeyboardState& getMidiKeyboardState();
    VoiceManagerState& getVoiceManagerState();

private:
    void setSirenId(sirenId id);
    //==========================================================================
    // int* midiMessageIntArray;
    // int* getIntFromMidiMessage(const void* data, int size);
    // => obsolete !
    // instead, use :
    // auto status = msg.getRawData()[0];
    // auto data1  = msg.getRawData()[1];
    // auto data2  = msg.getRawData()[2];
    // (see https://forum.juce.com/t/reading-midi-input/44721/5)

    // those are encapsulated in SirenVoice class now :
    // MidiIn* myMidiInHandler;
    // Synth* mySynth;

    // needed by DSP
    double lastSampleRate;
    int lastSamplesPerBlock;

    std::vector<parameterLayoutGroupData> parameterLayoutData;
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState midiKeyboardState;
    VoiceManagerState vms;

    MidiRouter router;
    MidiScheduler scheduler;
    std::atomic<SirenVoice*> currentSiren   { nullptr };
    std::atomic<SirenVoice*> discardedSiren { nullptr };
    std::atomic<bool> sirenIsLoading { false };

    std::function<std::string(void)> getResourcesPathFunction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OneSirenPluginProcessor)
};


#endif //ONESIREN_PLUGINPROCESSOR_H