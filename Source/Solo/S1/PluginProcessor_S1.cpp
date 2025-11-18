/*
  ==============================================================================

    PluginProcessor_S1.cpp
    ComposeSiren S1 Alto - Solo Plugin

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <functional>

// Variable globale pour accéder au processor depuis Sirene
SireneS1AudioProcessor* g_processor_s1 = nullptr;

//==============================================================================
SireneS1AudioProcessor::SireneS1AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    g_processor_s1 = this;
    startTimer(1);
    
    // Créer le synthé en mode Solo (uniquement S1)
    this->mySynth = new Synth(true, "S1");  // true = mode Solo
    
    auto onVelocityChanged =
        [this](int ch, int val)
        {
            if (ch == 1)
            {
                mySynth->setVelocite(1, val);  // Canal 1 pour S1 en mode Solo
                midiState.currentVelocity = juce::jlimit(0, 127, val);
            }
        };

    auto onEnginePitchChanged =
        [this](int ch, int val)
        {
            if (ch == 1)
            {
                mySynth->setVitesse(1, val);  // Canal 1 pour S1 en mode Solo
            }
        };

    myMidiInHandler = new MidiIn(onVelocityChanged, onEnginePitchChanged);
}

SireneS1AudioProcessor::~SireneS1AudioProcessor()
{
    delete myMidiInHandler;
    delete mySynth;
}

//==============================================================================
const juce::String SireneS1AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SireneS1AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SireneS1AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SireneS1AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SireneS1AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SireneS1AudioProcessor::getNumPrograms()
{
    return 1;
}

int SireneS1AudioProcessor::getCurrentProgram()
{
    return 0;
}

void SireneS1AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SireneS1AudioProcessor::getProgramName (int index)
{
    return {};
}

void SireneS1AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SireneS1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mySynth->setSampleRate(sampleRate);
    
    if (myMidiInHandler != nullptr)
    {
        myMidiInHandler->setSampleRate(sampleRate);
        myMidiInHandler->isWithSound(true);
    }
    
    sampleCountForMidiInTimer = 0;
}

void SireneS1AudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SireneS1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SireneS1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Traiter les messages MIDI
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        
        // Mettre à jour l'état MIDI pour l'affichage
        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            const int velocityValue = (msg.getRawDataSize() > 2)
                ? (static_cast<unsigned char>(msg.getRawData()[2]) & 0x7F)
                : juce::jlimit<int>(0, 127, msg.getVelocity());
            
            midiState.noteOn = true;
            midiState.currentNote = note;
            midiState.currentVelocity = velocityValue;
        }
        else if (msg.isNoteOff())
        {
            midiState.noteOn = false;
        }
        else if (msg.isController())
        {
            midiState.activeCC[msg.getControllerNumber()] = msg.getControllerValue();
        }
        
        if (myMidiInHandler != nullptr)
        {
            const int midiChannel = juce::jlimit<int>(1, 16, msg.getChannel()); // 1-based
            const int rawVelocity = (msg.getRawDataSize() > 2)
                ? (static_cast<unsigned char>(msg.getRawData()[2]) & 0x7F)
                : juce::jlimit<int>(0, 127, msg.getVelocity());
            
            if (msg.isNoteOn())
            {
            const int note = msg.getNoteNumber();
                const int velocityValue = rawVelocity;
                myMidiInHandler->RealTimeStartNote(midiChannel, note, velocityValue);
            }
            else if (msg.isNoteOff())
            {
                myMidiInHandler->RealTimeStopNote(midiChannel, msg.getNoteNumber());
            }
            else if (msg.isController())
            {
                myMidiInHandler->HandleControlChange(midiChannel, msg.getControllerNumber(), msg.getControllerValue());
            }
            else if (msg.isPitchWheel())
            {
                myMidiInHandler->HandlePitchWheel(midiChannel, msg.getPitchWheelValue() & 0x7F, (msg.getPitchWheelValue() >> 7) & 0x7F);
            }
        }
    }

    // Générer l'audio depuis le synth
    float* channelDataL = buffer.getWritePointer(0);
    float* channelDataR = buffer.getWritePointer(1);
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Appeler le timer audio tous les 512 samples
        if(sampleCountForMidiInTimer % 512 == 0)
        {
            myMidiInHandler->timerAudio();
        }
        ++sampleCountForMidiInTimer;
        
        float sampleS1 = 0.0f;
        
        // Générer l'audio de S1 uniquement (canal 1)
        // Pas de master volume ni pan - le DAW gère ces aspects
        if (mySynth->s1) {
            sampleS1 = mySynth->s1->calculwave();
        }
        
        // Sortie mono → stéréo (identique sur les deux canaux)
        // Le compositeur gère le panoramique dans son DAW
        channelDataL[sample] = sampleS1;
        channelDataR[sample] = sampleS1;
    }
}

//==============================================================================
bool SireneS1AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SireneS1AudioProcessor::createEditor()
{
    return new SireneS1AudioProcessorEditor (*this);
}

//==============================================================================
void SireneS1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Pas d'état à sauvegarder dans le plugin Solo
    // Le DAW gère les paramètres
}

void SireneS1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Pas d'état à restaurer
}

void SireneS1AudioProcessor::timerCallback()
{
    if (mySynth->s1) {
        mySynth->s1->setnote();
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SireneS1AudioProcessor();
}

