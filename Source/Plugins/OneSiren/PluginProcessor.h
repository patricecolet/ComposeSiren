//
// Created by joseph larralde on 20/02/2026.
//

#ifndef ONESIREN_PLUGINPROCESSOR_H
#define ONESIREN_PLUGINPROCESSOR_H

// #include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <MidiScheduler.h>
#include <apvtsUtilities.h>
#include <lib/wrappers/SirenVoice.h>
#include <Components/VoiceManagerState.h>
#include <Components/MainButtonsComponent.h>
#include "OneMidiRouter.h"
#include "lib/wrappers/SirenStateMonitor.h"
// #include "UiState.h"

class OneSirenPluginProcessor :
    public juce::AudioProcessor,
    public juce::Timer,
    public MainButtonsComponent::Listener,
    public VoiceManagerState::Listener
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

    // MainButtonsComponents::Listener callbacks
    //--------------------------------------------------------------------------
    void resetSiren(std::optional<sirenId>) override;
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

    const std::vector<parameterLayoutGroupData>& getParameterLayoutData() const;
    juce::AudioProcessorValueTreeState& getAudioProcessorValueTreeState();
    // UiState& getUiState();
    juce::MidiKeyboardState& getMidiKeyboardState();
    VoiceManagerState& getVoiceManagerState();
    SirenStateMonitor& getSirenStateMonitor();

private:
    // void initialiseUiState();
    void setSirenId(sirenId id);

    // needed by DSP
    double lastSampleRate;
    int lastBlockSize;

    std::vector<parameterLayoutGroupData> parameterLayoutData;
    juce::AudioProcessorValueTreeState apvts;
    // std::unique_ptr<UiState> uiState;
    juce::MidiKeyboardState midiKeyboardState;
    VoiceManagerState vms;

    OneMidiRouter router;
    MidiScheduler scheduler;
    SirenVoice siren;

    SirenStateMonitor ssm;

    std::function<std::string(void)> getResourcesPathFunction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OneSirenPluginProcessor)
};

#endif //ONESIREN_PLUGINPROCESSOR_H
