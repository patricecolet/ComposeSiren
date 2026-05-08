//
// Created by joseph larralde on 20/02/2026.
//

#ifndef SIRENORCHESTRA_PLUGINPROCESSOR_H
#define SIRENORCHESTRA_PLUGINPROCESSOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <Components/MainButtonsComponent.h>
#include <Components/VoiceManagerState.h>
#include <MidiScheduler.h>
#include <apvtsUtilities.h>
#include <Reverb.h>
#include <lib/wrappers/SirenEnsemble.h>
#include <lib/wrappers/SirenStateMonitor.h>
#include <ParameterBridges.h>
#include "OrchestraMidiRouter.h"

class SirenOrchestraPluginProcessor :
    public juce::AudioProcessor,
    public MainButtonsComponent::Listener,
    public VoiceManagerState::Listener,
    public juce::Timer
{
public:
    SirenOrchestraPluginProcessor();
    ~SirenOrchestraPluginProcessor() override;

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

    // Timer callback (called from UI thread)
    //--------------------------------------------------------------------------
    void timerCallback() override;

    // NB : FORCE CHANNEL ACCORDING TO SELECTED SIREN CATEGORY ?
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

    std::vector<parameterLayoutGroupData>& getParameterLayoutData();
    juce::AudioProcessorValueTreeState& getAudioProcessorValueTreeState();
    // UiState& getUiState();
    juce::MidiKeyboardState& getMidiKeyboardState();
    VoiceManagerState& getVoiceManagerState();
    SirenStateMonitor& getSirenStateMonitor();

private:
    // needed by DSP
    double lastSampleRate;
    int lastSamplesPerBlock;

    std::vector<parameterLayoutGroupData> parameterLayoutData;
    juce::AudioProcessorValueTreeState apvts;
    // std::unique_ptr<UiState> uiState;
    juce::MidiKeyboardState midiKeyboardState;
    VoiceManagerState vms;

    OrchestraMidiRouter router;
    MidiScheduler scheduler;

    std::function<std::string(void)> getResourcesPathFunction;

    SirenEnsemble ensemble;
    SirenEnsembleParameterBridges ensembleParameterBridges;

    Reverb reverb;
    ReverbParameterBridges reverbParameterBridges;

    SirenStateMonitor ssm;

private:
    //==========================================================================
    // int* midiMessageIntArray;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SirenOrchestraPluginProcessor)
};


#endif //SIRENORCHESTRA_PLUGINPROCESSOR_H