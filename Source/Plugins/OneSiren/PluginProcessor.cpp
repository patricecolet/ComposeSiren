//
// Created by joseph larralde on 20/02/2026.
//

#include <apvtsUtilities.h>
#include <pathUtilities.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

OneSirenPluginProcessor::OneSirenPluginProcessor() :
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
    lastBlockSize(512),
    parameterLayoutData(std::vector<parameterLayoutGroupData>{
          mkLayoutGroupData("S", "Siren", ParameterClass::SirenControl)
    }),
    apvts(*this,
          nullptr,
          "PARAMETERS",
          createParameterLayout(parameterLayoutData)
    ),
    router(parameterLayoutData[0], apvts, midiKeyboardState),
    getResourcesPathFunction(getResourcesPathGetter())
{
    vms.addListener(this);
    vms.notifyListeners();
    ssm.subscribe(&siren);
    startTimer(33);
}

OneSirenPluginProcessor::~OneSirenPluginProcessor()
{
    vms.removeListener(this);
    stopTimer();
}

//==============================================================================
void OneSirenPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;
    lastBlockSize = samplesPerBlock;

    siren.setSampleRate(sampleRate);
}

void OneSirenPluginProcessor::releaseResources()
{
    // playback stops, good place to release unused memory
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OneSirenPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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
//--------------------------------------------------------------------------
void OneSirenPluginProcessor::resetSiren(std::optional<sirenId>)
{
    siren.stop();
}

std::string OneSirenPluginProcessor::getResourcesPath()
{
    return getResourcesPathFunction();
}

void OneSirenPluginProcessor::selectedNewResourcesPath(const std::string& s)
{
    getResourcesPathFunction = getResourcesPathGetter(s);
    setSirenId(defaultSirenIdByCategory.at(vms.getSirenCategory()));
}

// VoiceManager callbacks :
// -----------------------------------------------------------------------------
void OneSirenPluginProcessor::categoryChanged(sirenCategory newCategory)
{
    setSirenId(defaultSirenIdByCategory.at(newCategory));
}

void OneSirenPluginProcessor::midiInputChanged(AnyOrOneBasedMidiChannel inch)
{
    router.setInputMidiChannel(inch);
}

void OneSirenPluginProcessor::midiOutputChanged(AnyOrOneBasedMidiChannel outch)
{
    router.setOutputMidiChannel(outch);
}

// juce::Timer callback :
// -----------------------------------------------------------------------------
void OneSirenPluginProcessor::timerCallback()
{
    // siren.update();
    siren.notifyListeners();
}

// void OneSirenPluginProcessor::initialiseUiState()
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

void OneSirenPluginProcessor::setSirenId(sirenId id)
{
    siren.setSirenId(id, getResourcesPath());
    router.sendAllCurrentParameterValues();
}

// NB : FORCE CHANNEL ACCORDING TO SELECTED SIREN CATEGORY ?
void OneSirenPluginProcessor::processBlock(juce::AudioBuffer<float>& audio,
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

    // internal rawSiren handle
    if (!siren.getRawSirenHandle()) {
        audio.clear();
        return;
    }

    // WE HAVE A SIREN SO WE PROCEED TO USE IT :

    audio.clear();
    auto* lch = audio.getWritePointer(0);
    auto* rch = audio.getWritePointer(1);

    siren.beginProcessBlock();

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

    // we can now safely delete siren's previous internal SirenVoiceUnit pointer
    // (if it's already nullptr, delete will just do nothing)
    siren.deleteDiscarded();
}

//==============================================================================
bool OneSirenPluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OneSirenPluginProcessor::createEditor()
{
    return new OneSirenPluginEditor(*this);
}


//==============================================================================
const juce::String OneSirenPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

double OneSirenPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

bool OneSirenPluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool OneSirenPluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool OneSirenPluginProcessor::isMidiEffect() const
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
int OneSirenPluginProcessor::getNumPrograms()
{
    // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
    return 1;
}

int OneSirenPluginProcessor::getCurrentProgram()
{
    return 0;
}

void OneSirenPluginProcessor::setCurrentProgram(int index)
{
}

const juce::String OneSirenPluginProcessor::getProgramName(int index)
{
    return {};
}

void OneSirenPluginProcessor::changeProgramName(int index,
                                                const juce::String& newName)
{
}

//==============================================================================
// Use an APVTS for plugin parameters, and another VTS for UI parameters
// (siren category, input MIDI channel, output MIDI channel)
//==============================================================================
void OneSirenPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xmlState("AllParameters");
    xmlState.addChildElement(apvts.state.createXml().release());
    xmlState.addChildElement(vms.toXml().release());
    copyXmlToBinary(xmlState, destData);
}

void OneSirenPluginProcessor::setStateInformation(const void* data,
                                                  int sizeInBytes)
{
    auto xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName("AllParameters")) {
            juce::XmlElement* xmlSubState;

            // ensure routing is restored before "one shot" parameters like
            // PitchBendRange are sent via the plugin's midi output
            xmlSubState = xmlState->getChildByName("VoiceManagerState");
            if (xmlSubState != nullptr) {
                vms.fromXml(*xmlSubState);
            }

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

const std::vector<parameterLayoutGroupData>&
OneSirenPluginProcessor::getParameterLayoutData() const
{
    return parameterLayoutData;
}

juce::AudioProcessorValueTreeState&
OneSirenPluginProcessor::getAudioProcessorValueTreeState()
{
    return apvts;
}

// UiState&
// OneSirenPluginProcessor::getUiState()
// {
//     return *uiState;
// }

juce::MidiKeyboardState& OneSirenPluginProcessor::getMidiKeyboardState()
{
    return midiKeyboardState;
}

VoiceManagerState& OneSirenPluginProcessor::getVoiceManagerState()
{
    return vms;
}

SirenStateMonitor& OneSirenPluginProcessor::getSirenStateMonitor()
{
    return ssm;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OneSirenPluginProcessor();
}
