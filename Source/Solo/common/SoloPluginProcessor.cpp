/*
  ==============================================================================

    SoloPluginProcessor.cpp
    ComposeSiren Solo Plugin (modèle paramétrable)

  ==============================================================================
*/

#include "SoloPluginProcessor.h"
#ifndef COMPOSESIREN_HEADLESS
#include "SoloPluginEditor.h"
#endif

#include <cmath>
#include <functional>
#include <sstream>

namespace
{
    constexpr int kTimerIntervalMs = 1;
}

//==============================================================================
SoloPluginAudioProcessor::SoloPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                    #if ! JucePlugin_IsMidiEffect
                     #if ! JucePlugin_IsSynth
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                    #endif
                      ),
      soloMidiChannel (SoloConfig::getMidiChannel()),
      sirenModel (SoloConfig::getModel())
#else
    : soloMidiChannel (SoloConfig::getMidiChannel()),
      sirenModel (SoloConfig::getModel())
#endif
{
    startTimer(kTimerIntervalMs);

    mySynth = new Synth(true, sirenModel.toStdString());
    activeSirene = resolveActiveSirene(soloMidiChannel);

    auto onVelocityChanged =
        [this](int ch, int val)
        {
            if (ch == soloMidiChannel && mySynth != nullptr)
            {
                mySynth->setVelocite(ch, val);
                midiState.currentVelocity = juce::jlimit(0, 127, val);
            }
        };

    auto onEnginePitchChanged =
        [this](int ch, int val)
        {
            if (ch == soloMidiChannel && mySynth != nullptr)
            {
                mySynth->setVitesse(ch, val);
            }
        };

    myMidiInHandler = new MidiIn(onVelocityChanged, onEnginePitchChanged);
    if (myMidiInHandler != nullptr)
        myMidiInHandler->isWithSound(true);
}

SoloPluginAudioProcessor::~SoloPluginAudioProcessor()
{
    delete myMidiInHandler;
    delete mySynth;
}

//==============================================================================
const juce::String SoloPluginAudioProcessor::getName() const
{
    return juce::String("ComposeSiren ") + SoloConfig::getDisplayName();
}

bool SoloPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SoloPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SoloPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SoloPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SoloPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int SoloPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SoloPluginAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String SoloPluginAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void SoloPluginAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void SoloPluginAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    if (mySynth != nullptr)
        mySynth->setSampleRate(sampleRate);
    
    if (myMidiInHandler != nullptr)
    {
        myMidiInHandler->setSampleRate(sampleRate);
        myMidiInHandler->isWithSound(true);
    }
    
    sampleCountForMidiInTimer = 0;
    activeSirene = resolveActiveSirene(soloMidiChannel);
}

void SoloPluginAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SoloPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SoloPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        
        // Utiliser la même méthode que Orchestra : convertir les données brutes en tableau
        midiMessageIntArray = getIntFromMidiMessage(msg.getRawData(), msg.getRawDataSize());
        int statusByte = midiMessageIntArray[0];
        int value1 = midiMessageIntArray[1];
        int value2 = midiMessageIntArray[2];
        
        // Mettre à jour midiState pour l'UI
        if (statusByte >= 144 && statusByte < 160) { // NoteOn
            if (value2 != 0) {
                midiState.noteOn = true;
                midiState.currentNote = value1;
                midiState.currentVelocity = value2;
            } else {
                midiState.noteOn = false;
            }
        } else if (statusByte >= 128 && statusByte < 144) { // NoteOff
            midiState.noteOn = false;
        } else if (statusByte >= 176 && statusByte < 192) { // Control Change
            midiState.activeCC[value1] = value2;
        }
        
        // Utiliser handleMIDIMessage2 comme dans Orchestra (mais filtrer par canal)
        if (myMidiInHandler != nullptr)
        {
            // Vérifier que le message est pour le bon canal
            int msgChannel = (statusByte & 0x0F) + 1; // Canal MIDI (1-16)
            if (msgChannel == soloMidiChannel || soloMidiChannel == 0) // 0 = omni
            {
                // Appeler handleMIDIMessage2 qui gère correctement la vélocité (comme Orchestra)
                myMidiInHandler->handleMIDIMessage2(statusByte, value1, value2);
            }
        }
    }

    float* channelDataL = buffer.getWritePointer(0);
    float* channelDataR = buffer.getWritePointer(1);
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        if (sampleCountForMidiInTimer % 512 == 0 && myMidiInHandler != nullptr)
            myMidiInHandler->timerAudio();

        ++sampleCountForMidiInTimer;
        
        float sampleValue = 0.0f;
        if (activeSirene != nullptr)
            sampleValue = activeSirene->calculwave();
        
        channelDataL[sample] = sampleValue;
        channelDataR[sample] = sampleValue;
    }
}

//==============================================================================
bool SoloPluginAudioProcessor::hasEditor() const
{
#ifdef COMPOSESIREN_HEADLESS
    return false;
#else
    return true;
#endif
}

juce::AudioProcessorEditor* SoloPluginAudioProcessor::createEditor()
{
#ifdef COMPOSESIREN_HEADLESS
    return nullptr;
#else
    return new SoloPluginAudioProcessorEditor (*this);
#endif
}

//==============================================================================
void SoloPluginAudioProcessor::getStateInformation (juce::MemoryBlock& /*destData*/)
{
}

void SoloPluginAudioProcessor::setStateInformation (const void* /*data*/, int /*sizeInBytes*/)
{
}

void SoloPluginAudioProcessor::timerCallback()
{
    if (activeSirene != nullptr)
        activeSirene->setnote();
}

Sirene* SoloPluginAudioProcessor::resolveActiveSirene(int channel) const
{
    if (mySynth == nullptr)
        return nullptr;

    switch (channel)
    {
        case 1: return mySynth->s1;
        case 2: return mySynth->s2;
        case 3: return mySynth->s3;
        case 4: return mySynth->s4;
        case 5: return mySynth->s5;
        case 6: return mySynth->s6;
        case 7: return mySynth->s7;
        default: return mySynth->s1;
    }
}

int* SoloPluginAudioProcessor::getIntFromMidiMessage(const void * data, int size)
// From a midi message and its size, output the midi message as an array of 3 integers
{
    static int arr[3];
    unsigned int x;

    juce::String hexaMessage =  juce::String::toHexString (data, size); // convert message to hexadecimal string

    juce::String value;
    int begin, end;
    // loop to split the string in 3 and convert each part in integer
    for (int i = 0; i < 3; ++i)
    {
        std::stringstream ss;
        begin = i*3;
        end = begin + 2;
        value = hexaMessage.substring(begin, end);
        ss << std::hex << value;
        ss >> x;
        arr[i] = static_cast<int>(x);
    }
    return arr;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoloPluginAudioProcessor();
}
