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
#ifndef COMPOSESIREN_PARK_BRIDGE
#define COMPOSESIREN_PARK_BRIDGE 0
#endif
#if COMPOSESIREN_PARK_BRIDGE
#include "SirenUdpBridge.h"
#endif
#include <lib/net/mcp/McpControl.h>
#include <lib/utilities/recorder/Recorder.h>
#ifndef COMPOSESIREN_CLIC
#define COMPOSESIREN_CLIC 0
#endif
#if COMPOSESIREN_CLIC
#include <clic_composesiren.hpp>
#include <lib/utilities/clic/ClicSecondaryOutput.h>
#include <cstdint>
#include <vector>
#endif
#include <lib/settings/Settings.h>
#include <juce_gui_basics/juce_gui_basics.h>

class SirenOrchestraPluginProcessor :
    public juce::AudioProcessor,
    public MainButtonsComponent::Listener,
    public juce::Timer
    , private cs::Settings::Listener
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
    std::atomic<unsigned> pendingResets{0}; // bit per sirenId, see resetSiren
    std::string getResourcesPath() override;
    void selectedNewResourcesPath(const std::string&) override;
#if COMPOSESIREN_PARK_BRIDGE
    void physicalSirensSwitched(bool) override;
    bool physicalSirensEnabled() override;
    juce::String physicalSirensTooltip() override;
    void stAllSwitched(bool) override;
#endif
#if COMPOSESIREN_RECORD
    Recorder* getRecorder() override { return &recorder; }
#endif
    bool hasAbout() override { return true; }
    void showAbout(juce::Component* parent) override;

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
#if COMPOSESIREN_PARK_BRIDGE
    SirenUdpBridge& getUdpBridge() { return udpBridge; }
#endif
    McpControl& getMcp() override { return mcp; }

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

#if COMPOSESIREN_PARK_BRIDGE
    // mirror du MIDI routé vers les sirènes physiques (protocole Pd sirenMidi2Udp)
    SirenUdpBridge udpBridge;
#endif

#if COMPOSESIREN_RECORD
    // before mcp, which drives it: destroyed after it
    Recorder recorder;
#endif

    McpControl mcp;

#if COMPOSESIREN_CLIC
    // the click box's engine, on the second output bus ("Clic")
    clic::Engine clicEngine { 44100.0 };
    // when set, the click plays on this device alone instead of the Clic bus
    ClicSecondaryOutput clicSecondary;
    std::atomic<float>* clicEnableParam = nullptr;
    std::atomic<float>* clicVolumeParam = nullptr;
    std::atomic<float>* clicSpreadParam = nullptr;
    std::atomic<float>* clicBiasParam = nullptr;
    std::atomic<float>* clicDecayParam = nullptr;
    // non-interleaved L/R scratch when the click goes only to clicSecondary
    std::vector<float> clicScratch;
#endif

    // the About window is not owned by the editor: it can outlive it
    juce::Component::SafePointer<juce::DialogWindow> aboutWindow;

private:
#if COMPOSESIREN_CLIC
    // channel 10 messages of the block, with their sample positions: taken
    // before the router (which has no siren on channel 10), played after
    // the sirens. Reserved in prepareToPlay: no allocation on the audio thread.
    struct ClicEvent
    {
        int position;
        std::uint8_t status, data1, data2;
    };
    std::vector<ClicEvent> clicEvents;
    void collectClicMidi(const juce::MidiBuffer& midi);
    void renderClic(juce::AudioBuffer<float>& audio);
    void applyClicOutputSetting();
#endif
    void settingChanged(cs::Settings::Id id) override;
#if COMPOSESIREN_RESETALLCONTROLLERS
    void applyControllerResetSetting();
    std::atomic<std::uint32_t> channelModePolicy { cs::DspChannelModePolicy{}.packed() };
#endif
    juce::SharedResourcePointer<cs::Settings> settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SirenOrchestraPluginProcessor)
};


#endif //SIRENORCHESTRA_PLUGINPROCESSOR_H
