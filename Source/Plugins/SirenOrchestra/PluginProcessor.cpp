//
// Created by joseph larralde on 20/02/2026.
//

#include <lib/definitions/sirenProperties.h>
#include <apvtsUtilities.h>
#include <pathUtilities.h>
#if COMPOSESIREN_RESETALLCONTROLLERS
#include <algorithm>
#endif
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AboutDialog.h"

SirenOrchestraPluginProcessor::SirenOrchestraPluginProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#if COMPOSESIREN_CLIC
        .withOutput("Clic", juce::AudioChannelSet::stereo(), true)
#endif
#endif
    ),
#endif
    // will be initialized by prepareToPlay at startup anyway
    lastSampleRate(44100.0),
    lastSamplesPerBlock(512),
    parameterLayoutData([]() {
        std::vector<parameterLayoutGroupData> res;
        for (const auto& sid : allSirenIds) {
            res.push_back(
                mkLayoutGroupData(sirenStrIdById.at(sid),
                                  sirenTitleById.at(sid),
                                  ParameterClass::SirenControl,
                                  ParameterClass::TrackControl)
            );
        }
        res.push_back(mkLayoutGroupData("R", "Reverb",
                                        ParameterClass::ReverbControl));
        res.push_back(mkLayoutGroupData("M", "Master",
                                        ParameterClass::MasterControl));
#if COMPOSESIREN_CLIC
        res.push_back(mkLayoutGroupData("C", "Clic",
                                        ParameterClass::ClicControl));
#endif
        return res;
    }()),
    apvts(*this,
          nullptr,
          "PARAMETERS",
          createParameterLayout(parameterLayoutData)
    ),
    router(parameterLayoutData, apvts, midiKeyboardState),
#if COMPOSESIREN_DSP_MIDI
    dspMidiRouter(apvts),
#endif
    getResourcesPathFunction(getResourcesPathGetter()),
    ensemble(allSirenIds, getResourcesPathFunction()),
    ensembleParameterBridges(apvts, &ensemble),
    reverbParameterBridges(apvts, &reverb),
    mcp(*this, "SirenOrchestra", "MvSO")
{
    ssm.subscribe(&ensemble);
#if COMPOSESIREN_RESETALLCONTROLLERS
    mcp.setResetHandler([this](int siren) {
        std::optional<sirenId> id;
        if (siren > 0) {
            const auto candidate = static_cast<sirenId>(siren - 1);
            if (std::find(allSirenIds.begin(), allSirenIds.end(), candidate) == allSirenIds.end())
                return false;
            id = candidate;
        }
        ensemble.requestReset(id);
#if COMPOSESIREN_PARK_BRIDGE
        if (id.has_value())
            udpBridge.pushReset(static_cast<int>(id.value()) + 1);
        else
            udpBridge.pushResetAll();
#endif
        return true;
    });
#endif
#if COMPOSESIREN_RECORD
    mcp.setRecorder(&recorder);
#endif
    router.sendAllCurrentParameterValues();
    ensembleParameterBridges.sendParameterValues();
    reverbParameterBridges.sendParameterValues();
#if COMPOSESIREN_CLIC
    clicEnableParam = apvts.getRawParameterValue(ParameterIdGet::toJuceParameterId("C", ParameterId::ClicEnable));
    clicVolumeParam = apvts.getRawParameterValue(ParameterIdGet::toJuceParameterId("C", ParameterId::ClicVolume));
    clicSpreadParam = apvts.getRawParameterValue(ParameterIdGet::toJuceParameterId("C", ParameterId::ClicSpread));
    clicBiasParam = apvts.getRawParameterValue(ParameterIdGet::toJuceParameterId("C", ParameterId::ClicBias));
    clicDecayParam = apvts.getRawParameterValue(ParameterIdGet::toJuceParameterId("C", ParameterId::ClicDecay));
#endif
    settings->addListener(this);
#if COMPOSESIREN_RESETALLCONTROLLERS
    applyControllerResetSetting();
#endif
#if COMPOSESIREN_CLIC
    applyClicOutputSetting();
#endif
#if COMPOSESIREN_DSP_MIDI
    dspMidiInput.setHardwareDevice(settings->getString(cs::Settings::dspMidiInputDevice));
#endif
    startTimer(33);
}

SirenOrchestraPluginProcessor::~SirenOrchestraPluginProcessor()
{
    settings->removeListener(this);
    stopTimer();
    // reads the MCP server and the bridge, which are destroyed with this object
    delete aboutWindow.getComponent();
}

void SirenOrchestraPluginProcessor::showAbout(juce::Component* parent)
{
    if (aboutWindow != nullptr)
        aboutWindow->toFront(true);
    else
        aboutWindow = AboutDialog::show(*this, parent);
}

//==============================================================================
void SirenOrchestraPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;
    lastSamplesPerBlock = samplesPerBlock;
#if COMPOSESIREN_RECORD
    recorder.setAudioFormat(sampleRate, getTotalNumOutputChannels());
#endif

    reverb.setSampleRate(sampleRate);
    ensemble.setSampleRate(sampleRate);
#if COMPOSESIREN_DSP_MIDI
    dspMidiInput.setSampleRate(sampleRate);
#endif
#if COMPOSESIREN_CLIC
    clicEngine.setSampleRate(sampleRate);
    clicEvents.reserve(512);
    clicScratch.assign(static_cast<size_t>(juce::jmax(samplesPerBlock, 256)) * 2, 0.0f);
#endif
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

#if COMPOSESIREN_CLIC
    // the Clic bus: stereo, or disabled
    if (layouts.outputBuses.size() > 1) {
        const auto& clicBus = layouts.outputBuses.getReference(1);
        if (!clicBus.isDisabled() && clicBus != juce::AudioChannelSet::stereo())
            return false;
    }
#endif

    return true;
#endif
}
#endif

// MainButtonsComponent::Listener callbacks
//------------------------------------------------------------------------------
#if COMPOSESIREN_PARK_BRIDGE
void SirenOrchestraPluginProcessor::physicalSirensSwitched(bool on)
{
    udpBridge.setEnabled(on);
}

bool SirenOrchestraPluginProcessor::physicalSirensEnabled()
{
    return udpBridge.isEnabled();
}

juce::String SirenOrchestraPluginProcessor::physicalSirensTooltip()
{
#if COMPOSESIREN_MECAVIV_BRIDGE
    return udpBridge.backendTooltip();
#else
    return "Uses SirenLink (in-process).";
#endif
}

void SirenOrchestraPluginProcessor::stAllSwitched(bool on)
{
    udpBridge.setStAll(on);
}
#endif

void SirenOrchestraPluginProcessor::resetSiren(std::optional<sirenId> id)
{
    ensemble.stop(id);

    // then the CC 121 itself, on the audio thread (processBlock)
    pendingResets.fetch_or(
        id.has_value() ? (1u << static_cast<int>(id.value())) : 0x7Fu,
        std::memory_order_release);

#if COMPOSESIREN_PARK_BRIDGE
    // relayer le reset aux sirènes physiques (trame [8, 10, 0...] du patch Pd).
    // The CC 121 above also reaches them through the MIDI mirror; the
    // CMD_RESET frame is the one known to work, and a second reset is harmless.
    if (id.has_value())
        udpBridge.pushReset(static_cast<int>(id.value()) + 1);
    else
        udpBridge.pushResetAll();
#endif
}

std::string SirenOrchestraPluginProcessor::getResourcesPath()
{
    return getResourcesPathFunction();
}

void SirenOrchestraPluginProcessor::selectedNewResourcesPath(const std::string& s)
{
    getResourcesPathFunction = getResourcesPathGetter();
    ensemble.updateResourcesPath(s);
    router.sendAllCurrentParameterValues();
    ensembleParameterBridges.sendParameterValues();
}

// juce::Timer callback
//------------------------------------------------------------------------------
void SirenOrchestraPluginProcessor::timerCallback()
{
    mcp.pump();
    // SirenStateMonitor fans out to editor components. Skip it while the
    // editor is absent or being destroyed (Cmd+Q teardown used to race this
    // into WavesOverflow's Component strings).
    if (uiActive.load(std::memory_order_acquire))
        ensemble.notifyListeners();
}

void SirenOrchestraPluginProcessor::beginEditorTeardown()
{
    uiActive.store(false, std::memory_order_release);
    mcp.setDispatchPaused(true);
}

void SirenOrchestraPluginProcessor::endEditorTeardown()
{
    mcp.setDispatchPaused(false);
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

void SirenOrchestraPluginProcessor::processBlock(juce::AudioBuffer<float>& audio,
                                                 juce::MidiBuffer& midiIn)
{
#if COMPOSESIREN_RESETALLCONTROLLERS
    const auto policyBits = channelModePolicy.load(std::memory_order_acquire);
    const auto policy = cs::DspChannelModePolicy::unpack(policyBits);
    router.setChannelModePolicy(policy);
    ensemble.setChannelModePolicy(policy);
#if COMPOSESIREN_PARK_BRIDGE
    const auto bridgePolicy = cs::BridgeChannelModePolicy::unpack(policyBits >> 7);
#endif
#endif
    juce::MidiBuffer midiOut;

    // MIDI ROUTING / SCHEDULING / UI SYNCING //////////////////////////////////

    mcp.drainMidi(midiIn);
    scheduler.reset();
#if COMPOSESIREN_DSP_MIDI
    {
        juce::MidiBuffer dspMidi;
        dspMidiInput.drain(dspMidi, audio.getNumSamples());
        for (const auto metadata : dspMidi)
            dspMidiRouter.handle(metadata.getMessage());
    }
#endif
#if COMPOSESIREN_CLIC
    collectClicMidi(midiIn);
#endif

    // the Reset buttons: the same as a CC 121 on the siren's channel (or 16)
    if (const auto mask = pendingResets.exchange(0, std::memory_order_acquire)) {
        for (const auto& [id, props] : sirenPropertiesById) {
            if (mask & (1u << static_cast<int>(id)))
                router.resetSirens(scheduler, props->oneBasedMidiChannel, 127, 0);
        }
    }

    for (const auto metadata : midiIn) {
        const auto& msg = metadata.getMessage();
        // - discard unknown CC messages
        // - discard messages that don't match the input MIDI channel
        // - (except if input channel is AnyMidiChannel)
        // - keep incoming MIDI events with precise timing
        // - dump them as is into scheduler
        // - forward them to UI for monitoring (with scoped guards)
        router.handleMessage(scheduler, msg, metadata.samplePosition);
    }

    // schedule MIDI output from UI/host control
    router.processBridges(scheduler, audio.getNumSamples());

    // flush scheduler to MIDI output
    scheduler.flush(midiOut);

    // ... THEN
    midiIn.swapWith(midiOut);

#if COMPOSESIREN_PARK_BRIDGE
    // mirror du MIDI routé vers les sirènes physiques via UDP
    // (lock-free : les envois réseau se font sur le thread du bridge)
    for (const auto metadata : midiIn) {
        const auto& m = metadata.getMessage();
        if (m.getRawDataSize() == 3) {
#if COMPOSESIREN_RESETALLCONTROLLERS
            if (m.isController()) {
                const auto value = bridgePolicy.forwardedValue(m.getControllerNumber(), m.getControllerValue());
                if (value.has_value())
                    udpBridge.pushMidi(m.getRawData()[0], m.getRawData()[1], static_cast<std::uint8_t>(*value));
                continue;
            }
#endif
            udpBridge.pushMidi(m.getRawData()[0],
                               m.getRawData()[1],
                               m.getRawData()[2]);
        }
    }
#endif

    // AUDIO CONTROL / SYNTHESIS ///////////////////////////////////////////////

    if (!ensemble.getRawSirenHandles()) {
        // a siren still loading: its controllers (the knobs, a restored state) are kept
        // for it, not lost with this silent block
        for (const auto metadata : midiIn) {
            const auto* d = metadata.getMessage().getRawData();
            if (metadata.getMessage().getRawDataSize() == 3) ensemble.handleMidiWithoutHandles(d[0], d[1], d[2]);
        }
        audio.clear();
#if COMPOSESIREN_CLIC
        renderClic(audio);
#endif
        return;
    }

    // WE HAVE ALL SIRENS SO WE PROCEED TO USE THEM :

    audio.clear();
    auto* lch = audio.getWritePointer(0);
    auto* rch = audio.getWritePointer(1);

    ensemble.beginProcessBlock();
    reverb.beginProcessBlock();

    juce::MidiBufferIterator midiIt = midiIn.findNextSamplePosition(0);
    juce::MidiMessageMetadata metadata;
    int nextPosition = -1;
    if (midiIt != midiIn.cend()) {
        metadata = *midiIt;
        nextPosition = metadata.samplePosition;
    }

    // sirens are stateful, so we must send the midi messages sample-wise
    // and call process() on each sample
    for (int i = 0; i < audio.getNumSamples(); ++i) {
        while (nextPosition == i) {
            auto msg = metadata.getMessage();
            ensemble.handleMidi(msg.getRawData()[0],
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

        float l, r;
        ensemble.process(&l, &r);
        reverb.process(&l, &r, &lch[i], &rch[i], 1);
    }

    // security inspired from SurgeSynthProcessor.cpp
    // "this should never happen but better safe than sorry"
    // -> flush all pending messages into siren
    while (midiIt != midiIn.cend()) {
        metadata = *midiIt;
        auto msg = metadata.getMessage();
        ensemble.handleMidi(msg.getRawData()[0],
                            msg.getRawData()[1],
                            msg.getRawData()[2]);
        ++midiIt;
    }

    // now we can safely delete the previous siren pointer
    // (if it's already nullptr, delete will just do nothing)
    ensemble.deleteDiscarded();

#if COMPOSESIREN_CLIC
    renderClic(audio);
#endif

#if COMPOSESIREN_RECORD
    // what goes out, after the reverb: copied to the recorder's ring only
    recorder.process(audio);
#endif
}

#if COMPOSESIREN_CLIC
void SirenOrchestraPluginProcessor::collectClicMidi(const juce::MidiBuffer& midi)
{
    clicEvents.clear();
    for (const auto metadata : midi) {
        const auto& m = metadata.getMessage();
        const auto* raw = m.getRawData();
        if (m.getRawDataSize() < 2 || (raw[0] & 0xf0) == 0xf0 || (raw[0] & 0x0f) != 9)
            continue; // channel 10 only
        if (clicEvents.size() == clicEvents.capacity())
            break; // never reallocate on the audio thread
        clicEvents.push_back({ metadata.samplePosition, raw[0], raw[1],
                               static_cast<std::uint8_t>(m.getRawDataSize() > 2 ? raw[2] : 0) });
    }
}

void SirenOrchestraPluginProcessor::applyClicOutputSetting()
{
    clicSecondary.setDevice(settings->getString(cs::Settings::Id::clicOutputDevice));
}

void SirenOrchestraPluginProcessor::renderClic(juce::AudioBuffer<float>& audio)
{
    if (getBusCount(false) < 2)
        return;
    auto bus = getBusBuffer(audio, false, 1);
    const int n = bus.getNumSamples();
    // ClicEnable is a master override: OFF ⇒ silent regardless of ClicVolume
    const bool enabled = clicEnableParam == nullptr
                             || clicEnableParam->load(std::memory_order_relaxed) > 0.5f;
    const float knobVol = clicVolumeParam != nullptr ? clicVolumeParam->load(std::memory_order_relaxed) : 1.0f;
    const float vol = enabled ? knobVol : 0.0f;
    const float spread = clicSpreadParam != nullptr ? clicSpreadParam->load(std::memory_order_relaxed) : 0.0f;
    const float bias = clicBiasParam != nullptr ? clicBiasParam->load(std::memory_order_relaxed) : 0.0f;
    const float decay = clicDecayParam != nullptr ? clicDecayParam->load(std::memory_order_relaxed) : 1.0f;
    const bool secondary = clicSecondary.isActive();
    const bool toBus = bus.getNumChannels() >= 2 && !secondary;
    if (secondary && bus.getNumChannels() > 0)
        bus.clear(); // the click plays alone on the secondary device

    // scratch on the bus when it is silent, or on a reserved buffer when the
    // click only goes to the secondary device (the bus must stay silent then)
    float* l = nullptr;
    float* r = nullptr;
    if (toBus) {
        l = bus.getWritePointer(0);
        r = bus.getWritePointer(1);
    } else if (secondary) {
        if (clicScratch.size() < static_cast<size_t>(n) * 2)
            clicScratch.assign(static_cast<size_t>(n) * 2, 0.0f); // prepareToPlay undersized
        l = clicScratch.data();
        r = clicScratch.data() + n;
    } else {
        // bus disabled and no secondary device: events still reach the engine
        for (const auto& e : clicEvents)
            clicEngine.midi(e.status, e.data1, e.data2);
        return;
    }

    int done = 0;
    for (const auto& e : clicEvents) {
        const int at = juce::jlimit(done, n, e.position);
        clicEngine.renderPanned(l + done, r + done, at - done, spread, bias, decay);
        clicEngine.midi(e.status, e.data1, e.data2);
        done = at;
    }
    clicEngine.renderPanned(l + done, r + done, n - done, spread, bias, decay);

    if (vol != 1.0f) {
        juce::FloatVectorOperations::multiply(l, vol, n);
        juce::FloatVectorOperations::multiply(r, vol, n);
    }
    if (secondary)
        clicSecondary.push(l, r, n);
}
#endif

#if COMPOSESIREN_RESETALLCONTROLLERS
void SirenOrchestraPluginProcessor::applyControllerResetSetting()
{
    const auto policy = settings->getDspChannelModePolicy();
    auto bits = policy.packed();
#if COMPOSESIREN_PARK_BRIDGE
    bits |= settings->getBridgeChannelModePolicy().packed() << 7;
#endif
    channelModePolicy.store(bits, std::memory_order_release);
    ensemble.setResetRequestsEnabled(policy.enabled);
    mcp.setControllerResetEnabled(policy.enabled);
}
#endif

void SirenOrchestraPluginProcessor::settingChanged(cs::Settings::Id id)
{
#if COMPOSESIREN_RESETALLCONTROLLERS
    if (cs::Settings::isControllerResetSetting(id))
        applyControllerResetSetting();
#endif
#if COMPOSESIREN_RUST_DSP
    // app-wide: every siren rebuilt off the audio thread, swapped at its next free note-on
    if (id == cs::Settings::Id::dspVariant || id == cs::Settings::Id::dspDataset
        || id == cs::Settings::Id::dspSkipSilentFrames)
        ensemble.reloadDsp();
#endif
#if COMPOSESIREN_CLIC
    if (id == cs::Settings::Id::clicOutputDevice)
        applyClicOutputSetting();
#endif
#if COMPOSESIREN_DSP_MIDI
    if (id == cs::Settings::dspMidiInputDevice)
        dspMidiInput.setHardwareDevice(settings->getString(id));
#endif
#if !COMPOSESIREN_CLIC && !COMPOSESIREN_DSP_MIDI && !COMPOSESIREN_RESETALLCONTROLLERS && !COMPOSESIREN_RUST_DSP
    juce::ignoreUnused(id);
#endif
}

//==============================================================================
bool SirenOrchestraPluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SirenOrchestraPluginProcessor::createEditor()
{
    uiActive.store(true, std::memory_order_release);
    mcp.setDispatchPaused(false);
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
    // copyState flushes the parameters into the tree first (APVTS does it on a timer)
    xmlState.addChildElement(apvts.copyState().createXml().release());
    xmlState.addChildElement(vms.toXml().release());
#if COMPOSESIREN_PARK_BRIDGE
    // le pilotage des sirènes physiques suit la session, pas le plugin
    auto* bridgeXml = xmlState.createNewChildElement("UdpBridge");
    bridgeXml->setAttribute("enabled", udpBridge.isEnabled());
#endif
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

#if COMPOSESIREN_PARK_BRIDGE
            xmlSubState = xmlState->getChildByName("UdpBridge");
            if (xmlSubState != nullptr) {
                udpBridge.setEnabled(xmlSubState->getBoolAttribute("enabled", false));
            }
#endif
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

SirenStateMonitor& SirenOrchestraPluginProcessor::getSirenStateMonitor()
{
    return ssm;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SirenOrchestraPluginProcessor();
}
