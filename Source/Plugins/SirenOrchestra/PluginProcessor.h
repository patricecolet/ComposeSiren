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
#include "SirenUdpBridge.h"

class SirenOrchestraPluginProcessor :
    public juce::AudioProcessor,
    public MainButtonsComponent::Listener,
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

    // MainButtonsComponents::Listener callbacks
    //--------------------------------------------------------------------------
    void resetSiren(std::optional<sirenId>) override;
    std::string getResourcesPath() override;
    void selectedNewResourcesPath(const std::string&) override;
    void stAllSwitched(bool) override;

    // Timer callback (called from UI thread)
    //--------------------------------------------------------------------------
    void timerCallback() override;

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
    SirenUdpBridge& getUdpBridge() { return udpBridge; }

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

    // mirror du MIDI routé vers les sirènes physiques (protocole Pd sirenMidi2Udp)
    SirenUdpBridge udpBridge;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SirenOrchestraPluginProcessor)
};


#endif //SIRENORCHESTRA_PLUGINPROCESSOR_H