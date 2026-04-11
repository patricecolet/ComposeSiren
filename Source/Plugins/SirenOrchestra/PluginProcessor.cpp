//
// Created by joseph larralde on 20/02/2026.
//

#include <apvtsUtilities.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

SirenOrchestraPluginProcessor::SirenOrchestraPluginProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    // will be initialized by prepareToPlay at startup anyway
    lastSampleRate(44100.0),
    lastSamplesPerBlock(512),
    parameterLayoutData(std::vector<parameterLayoutGroupData>{
        mkLayoutGroupData("S1", "Alto 1",
                          ParameterClass::SirenControl,
                          ParameterClass::TrackControl),
        mkLayoutGroupData("S2", "Alto 2",
                          ParameterClass::SirenControl,
                          ParameterClass::TrackControl),
        mkLayoutGroupData("S3", "Basse",
                          ParameterClass::SirenControl,
                          ParameterClass::TrackControl),
        mkLayoutGroupData("S4", "Tenor",
                          ParameterClass::SirenControl,
                          ParameterClass::TrackControl),
        mkLayoutGroupData("S5", "Soprano 1",
                          ParameterClass::SirenControl,
                          ParameterClass::TrackControl),
        mkLayoutGroupData("S6", "Soprano 2",
                          ParameterClass::SirenControl,
                          ParameterClass::TrackControl),
        mkLayoutGroupData("S7", "Piccolo",
                          ParameterClass::SirenControl,
                          ParameterClass::TrackControl),
        mkLayoutGroupData("R",  "Reverb",
                          ParameterClass::ReverbControl),
        mkLayoutGroupData("M",  "Master",
                          ParameterClass::MasterControl)
    }),
    apvts(*this,
          nullptr,
          "PARAMETERS",
          createParameterLayout(parameterLayoutData)
    ),
    // Reminder : we want one MidiRouter instance per siren group
    router(parameterLayoutData[0], apvts, midiKeyboardState),
    getResourcesPathFunction(getResourcesPathGetter())
{
    // vms.addListener(this);
    // const sirenCategory defaultSirenCategory = vms.getSirenCategory();
    // setSirenId(sirenPropertiesByCategory.at(defaultSirenCategory)->id);
    // startTimer(1);
}

SirenOrchestraPluginProcessor::~SirenOrchestraPluginProcessor()
{
    // vms.removeListener(this);
}

//==============================================================================
void SirenOrchestraPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;
    lastSamplesPerBlock = samplesPerBlock;

    // if (sirenIsLoading.load(std::memory_order_acquire)) { return; }
    // auto siren = currentSiren.load(std::memory_order_acquire);
    // if (siren == nullptr) { return; }
    siren.setSampleRate(sampleRate);
}

void SirenOrchestraPluginProcessor::releaseResources()
{
    // playback stops, good place to release unused memory
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SirenOrchestraPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

// MainButtonsComponent::Listener callbacks
//------------------------------------------------------------------------------
void SirenOrchestraPluginProcessor::resetSiren()
{
    // if (sirenIsLoading.load(std::memory_order_acquire)) { return; }
    // auto siren = currentSiren.load(std::memory_order_acquire);
    // if (siren == nullptr) { return; }
    siren.stop();
}

std::string SirenOrchestraPluginProcessor::getResourcesPath()
{
    return getResourcesPathFunction();
}

void SirenOrchestraPluginProcessor::selectedNewResourcesPath(const std::string& s)
{
    getResourcesPathFunction = getResourcesPathGetter();
    // setSirenId(defaultSirenIdByCategory.at(vms.getSirenCategory()));
}

// juce::Timer callback
//------------------------------------------------------------------------------
void SirenOrchestraPluginProcessor::timerCallback()
{
    // if (sirenIsLoading.load(std::memory_order_acquire)) { return; }
    // auto siren = currentSiren.load(std::memory_order_acquire);
    // if (siren == nullptr) { return; }
    siren.update();
}

// void SirenOrchestraPluginProcessor::initialiseUiState()
// {
//     auto uiTree = apvts.state.getChildWithName("UISTATE");
//     if (!uiTree.isValid())
//     {
//         uiTree = UiState::createDefaultState();
//         apvts.state.addChild(uiTree, -1, nullptr);
//     }
//     uiState = std::make_unique<UiState>(uiTree);
// }

//==============================================================================
// PROCESS BLOCK
//==============================================================================

// void SirenOrchestraPluginProcessor::setSirenId(sirenId id)
// {
//     // try to use shared_ptr instead, see :
//     // https://www.modernescpp.com/index.php/a-lock-free-stack-atomic-smart-pointer/
//     sirenIsLoading.store(true, std::memory_order_release);
//     auto* newSiren = new SirenVoice(id, getResourcesPath());
//     newSiren->setSampleRate(lastSampleRate);
//     auto* oldSiren = currentSiren.exchange(newSiren, std::memory_order_acq_rel);
//     // safe to delete the old Siren at the end of processBlock, the audio thread
//     // might still be using it right now (think in terms of ownership horizon)
//     discardedSiren.store(oldSiren, std::memory_order_release);
//     sirenIsLoading.store(false, std::memory_order_release);
//     router.sendAllCurrentParameterValues();
// }

// NB : FORCE CHANNEL ACCORDING TO SELECTED SIREN CATEGORY ?
void SirenOrchestraPluginProcessor::processBlock(juce::AudioBuffer<float>& audio,
                                           juce::MidiBuffer& midiIn)
{
    juce::MidiBuffer midiOut;

    // MIDI ROUTING / SCHEDULING / UI SYNCING //////////////////////////////////

    scheduler.reset();

    for (const auto metadata : midiIn) {
        const auto& msg = metadata.getMessage();
        // discard unknown CC messages
        // discard messages that don't match the input MIDI channel
        // (except if input channel is AnyMidiChannel)
        // keep incoming MIDI events with precise timing
        // dump them as is into scheduler
        // forward them to UI for monitoring
        router.handleMessage(scheduler, msg, metadata.samplePosition);
    }

    // schedule MIDI output from UI/host control
    router.processBridges(scheduler, audio.getNumSamples());

    // flush scheduler to MIDI output
    scheduler.flush(midiOut);

    // ... THEN
    midiIn.swapWith(midiOut);

    // AUDIO CONTROL / SYNTHESIS ///////////////////////////////////////////////

    // if (sirenIsLoading.load(std::memory_order_acquire)) {
    //     audio.clear();
    //     return;
    // }
    //
    // auto siren = currentSiren.load(std::memory_order_acquire);
    //
    // if (siren == nullptr) {
    //     audio.clear();
    //     return;
    // }

    if (!siren.getRawSirenHandle()) {
        audio.clear();
        return;
    }

    // WE HAVE A SIREN SO WE PROCEED TO USE IT :

    audio.clear();
    auto* lch = audio.getWritePointer(0);
    auto* rch = audio.getWritePointer(1);

    juce::MidiBufferIterator midiIt = midiIn.findNextSamplePosition(0);
    juce::MidiMessageMetadata metadata;
    int nextPosition = -1;
    if (midiIt != midiIn.cend()) {
        metadata = *midiIt;
        nextPosition = metadata.samplePosition;
    }

    // siren is stateful so we must send the midi messsages at the right samples
    // and call process() on each sample
    for (int i = 0; i < audio.getNumSamples(); ++i) {
        while (nextPosition == i) {
            auto msg = metadata.getMessage();
            siren.handleMidi(msg.getRawData()[0],
                             msg.getRawData()[1],
                             msg.getRawData()[2]);

            ++midiIt;
            if (midiIt == midiIn.cend()) {
                nextPosition = -1;
            } else {
                metadata = *midiIt;
                nextPosition = metadata.samplePosition;
            }
        }

        float sample = siren.process();
        lch[i] = rch[i] = sample;
    }

    // security inspired from SurgeSynthProcessor.cpp
    // "this should never happen but better safe than sorry"
    // -> flush all pending messages into siren
    while (midiIt != midiIn.cend()) {
        metadata = *midiIt;
        auto msg = metadata.getMessage();
        siren.handleMidi(msg.getRawData()[0],
                         msg.getRawData()[1],
                         msg.getRawData()[2]);
        ++midiIt;
    }

    // now we can safely delete the previous siren pointer
    // (if it's already nullptr, delete will just do nothing)
    // delete discardedSiren.exchange(nullptr);
    siren.deleteDiscarded();
}

//==============================================================================
bool SirenOrchestraPluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SirenOrchestraPluginProcessor::createEditor()
{
    return new SirenOrchestraPluginEditor(*this);
}


//==============================================================================
const juce::String SirenOrchestraPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

double SirenOrchestraPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

bool SirenOrchestraPluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SirenOrchestraPluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SirenOrchestraPluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

//==============================================================================
// We don't use programs for now
//==============================================================================
int SirenOrchestraPluginProcessor::getNumPrograms()
{
    // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
    return 1;
}

int SirenOrchestraPluginProcessor::getCurrentProgram()
{
    return 0;
}

void SirenOrchestraPluginProcessor::setCurrentProgram(int index)
{
}

const juce::String SirenOrchestraPluginProcessor::getProgramName(int index)
{
    return {};
}

void SirenOrchestraPluginProcessor::changeProgramName(int index,
                                                const juce::String& newName)
{
}

//==============================================================================
// Use an APVTS for plugin parameters, and another VTS for UI parameters
// (siren category, input MIDI channel, output MIDI channel)
//==============================================================================
void SirenOrchestraPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xmlState("AllParameters");
    xmlState.addChildElement(apvts.state.createXml().release());
    // xmlState.addChildElement(vms.toXml().release());
    copyXmlToBinary(xmlState, destData);
}

void SirenOrchestraPluginProcessor::setStateInformation(const void* data,
                                                  int sizeInBytes)
{
    auto xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName("AllParameters")) {
            juce::XmlElement* xmlSubState;

            // ensure routing is restored before "one shot" parameters like
            // PitchBendRange are sent via the plugin's midi output
            // xmlSubState = xmlState->getChildByName("VoiceManagerState");
            // if (xmlSubState != nullptr) {
            //     vms.fromXml(*xmlSubState);
            // }

            // now the apvts can trigger midi output messages which will be routed
            // correctly
            xmlSubState = xmlState->getChildByName(apvts.state.getType());
            if (xmlSubState != nullptr) {
                apvts.replaceState(juce::ValueTree::fromXml(*xmlSubState));
            }
        }
    }

    // If we integrate UiParameters as another valueTree in apvts : ------------
    // if (xml != nullptr) {
    //     auto tree = juce::ValueTree::fromXml(*xml);
    //
    //     if (tree.isValid()) {
    //         apvts.replaceState(tree);
    //         // initialiseUiState();
    //     }
    // }
}

std::vector<parameterLayoutGroupData>&
SirenOrchestraPluginProcessor::getParameterLayoutData()
{
    return parameterLayoutData;
}

juce::AudioProcessorValueTreeState&
SirenOrchestraPluginProcessor::getAudioProcessorValueTreeState()
{
    return apvts;
}

// UiState&
// SirenOrchestraPluginProcessor::getUiState()
// {
//     return *uiState;
// }

juce::MidiKeyboardState& SirenOrchestraPluginProcessor::getMidiKeyboardState()
{
    return midiKeyboardState;
}

VoiceManagerState& SirenOrchestraPluginProcessor::getVoiceManagerState()
{
    return vms;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SirenOrchestraPluginProcessor();
}
