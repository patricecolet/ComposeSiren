/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <functional>

// Constante pour atténuer S7 (piccolo) qui est trop fort
static const float S7_ATTENUATION = 0.3f;

// Variable globale pour accéder au processor depuis Sirene
SirenePlugAudioProcessor* g_processor = nullptr;

//==============================================================================
SirenePlugAudioProcessor::SirenePlugAudioProcessor()
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
    g_processor = this; // Assigner l'instance courante
    startTimer(1);
    this->mySynth = new Synth();
    
    auto onVelocityChanged =
        [this](int ch, int val)
        {
	        mySynth->setVelocite(ch, val);
        };

    auto onEnginePitchChanged =
        [this](int ch, int val)
        {
            mySynth->setVitesse(ch, val);
        };

    myMidiInHandler = new MidiIn(onVelocityChanged, onEnginePitchChanged);

}

SirenePlugAudioProcessor::~SirenePlugAudioProcessor()
{
}

//==============================================================================
const juce::String SirenePlugAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SirenePlugAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SirenePlugAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SirenePlugAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SirenePlugAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SirenePlugAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SirenePlugAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SirenePlugAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SirenePlugAudioProcessor::getProgramName (int index)
{
    return {};
}

void SirenePlugAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SirenePlugAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Propager le sample rate aux composants qui en ont besoin
    if (mySynth != nullptr) {
        mySynth->setSampleRate(sampleRate);
    }
    
    if (myMidiInHandler != nullptr) {
        myMidiInHandler->setSampleRate(sampleRate);
    }
}

void SirenePlugAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SirenePlugAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SirenePlugAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Supprimer les messages de debug trop fréquents qui causent des plantages
    
    buffer.clear();

    // process midi message
    for (const auto meta : midiMessages) {
        const auto msg = meta.getMessage();
        midiMessageIntArray = getIntFromMidiMessage(msg.getRawData(), msg.getRawDataSize());
        
        // Gérer les Control Change pour mixeur et reverb
        int statusByte = midiMessageIntArray[0];
        int ccNumber = midiMessageIntArray[1];
        int ccValue = midiMessageIntArray[2];
        
        if (statusByte >= 176 && statusByte < 192) { // Control Change messages
            int channel = statusByte - 175; // Canal MIDI (1-16)
            
            if (channel >= 1 && channel <= 7) { // Canaux 1-7 : sirènes individuelles
                if (ccNumber == 10) { // CC10 = Pan
                    float pan = (ccValue / 127.0f) - 0.5f;
                    mySynth->setPan(channel, pan);
                }
                else if (ccNumber == 70) { // CC70 = Master Volume indépendant
                    float volume = ccValue / 127.0f;
                    mySynth->setMasterVolume(channel, volume);
                }
            }
            else if (channel == 16) { // Canal 16 : contrôles reverb globale
                switch (ccNumber) {
                    case 64: // Enable reverb
                        mySynth->setReverbEnabled(ccValue >= 64);
                        break;
                    case 65: // Room Size
                        mySynth->reverb->setroomsize(ccValue / 127.0f);
                        break;
                    case 66: // Dry/Wet
                        {
                            float dryWet = ccValue / 127.0f;
                            mySynth->reverb->setwet(dryWet);
                            mySynth->reverb->setdry(1.0f - dryWet);
                        }
                        break;
                    case 67: // Damp
                        mySynth->reverb->setdamp(ccValue / 127.0f);
                        break;
                    case 68: // Highpass (20-2000 Hz)
                        {
                            float freq = 20.0f + (ccValue / 127.0f) * 1980.0f;
                            mySynth->setReverbHighpass(freq);
                        }
                        break;
                    case 69: // Lowpass (2kHz-20kHz)
                        {
                            float freq = 2000.0f + (ccValue / 127.0f) * 18000.0f;
                            mySynth->setReverbLowpass(freq);
                        }
                        break;
                    case 70: // Width
                        mySynth->reverb->setwidth(ccValue / 127.0f);
                        break;
                }
            }
        }
        
        myMidiInHandler->handleMIDIMessage2(midiMessageIntArray[0], midiMessageIntArray[1], midiMessageIntArray[2]);

    }

    float sampleS1 = 0;
    float sampleS2 = 0;
    float sampleS3 = 0;
    float sampleS4 = 0;
    float sampleS5 = 0;
    float sampleS6 = 0;
    float sampleS7 = 0;

    juce::ScopedNoDenormals noDenormals;

    auto* channelLeft = buffer.getWritePointer(0);
    auto* channelRight = buffer.getWritePointer(1);

    for (auto sample = 0; sample < buffer.getNumSamples(); sample++) {
        // in original code, this timer updating dsp computations related to note slide, vibrato & tremolo is
        // only called once every block, and block is hardcoded to be 512
        // implement this with a counter
        if(sampleCountForMidiInTimer % 512 == 0)
        {
            myMidiInHandler->timerAudio();
        }
        ++sampleCountForMidiInTimer;

        // Calculer les samples de chaque sirène
        sampleS1 = mySynth->s1->calculwave();
        sampleS2 = mySynth->s2->calculwave();
        sampleS3 = mySynth->s3->calculwave();
        sampleS4 = mySynth->s4->calculwave();
        sampleS5 = mySynth->s5->calculwave();
        sampleS6 = mySynth->s6->calculwave();
        sampleS7 = mySynth->s7->calculwave();
        
        // Appliquer le master volume (CC70) - multiplicatif avec le volume original
        sampleS1 *= mySynth->getMasterVolume(1);
        sampleS2 *= mySynth->getMasterVolume(2);
        sampleS3 *= mySynth->getMasterVolume(3);
        sampleS4 *= mySynth->getMasterVolume(4);
        sampleS5 *= mySynth->getMasterVolume(5);
        sampleS6 *= mySynth->getMasterVolume(6);
        sampleS7 *= mySynth->getMasterVolume(7);

        // Mixer avec panoramique
        channelLeft[sample] =
            sampleS1 * mySynth->getPan(1,0) +
            sampleS2 * mySynth->getPan(2,0) +
            sampleS3 * mySynth->getPan(3,0) +
            sampleS4 * mySynth->getPan(4,0) +
            sampleS5 * mySynth->getPan(5,0) +
            sampleS6 * mySynth->getPan(6,0) +
            sampleS7 * mySynth->getPan(7,0) * S7_ATTENUATION;

        channelRight[sample] =
            sampleS1 * mySynth->getPan(1,1) +
            sampleS2 * mySynth->getPan(2,1) +
            sampleS3 * mySynth->getPan(3,1) +
            sampleS4 * mySynth->getPan(4,1) +
            sampleS5 * mySynth->getPan(5,1) +
            sampleS6 * mySynth->getPan(6,1) +
            sampleS7 * mySynth->getPan(7,1) * S7_ATTENUATION;
        
        // TODO: Investiguer pourquoi le niveau audio est ~1000x trop faible sur Linux
        // Fix temporaire : gain de compensation sur Linux uniquement
        #if defined(__linux__) || defined(__unix__)
        const float LINUX_OUTPUT_GAIN = 50.0f;
        channelLeft[sample] *= LINUX_OUTPUT_GAIN;
        channelRight[sample] *= LINUX_OUTPUT_GAIN;
        #endif

        if(channelLeft[sample] != 0) {
            ;
        }
    }
    
    // Appliquer la reverb avec filtres si activée
    if(buffer.getNumSamples() > 0) {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        mySynth->processReverbWithFilters(left, right, buffer.getNumSamples());
    }
}

//==============================================================================
bool SirenePlugAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SirenePlugAudioProcessor::createEditor()
{
    return new SirenePlugAudioProcessorEditor (*this);
}

//==============================================================================
void SirenePlugAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Sauvegarder les paramètres du mixeur et de la reverb
    juce::ValueTree state("MixerState");
    
    // Sauvegarder les pans
    for (int i = 1; i <= 7; i++)
    {
        // getPan retourne une valeur de 0.5 à 1.5, donc on sauvegarde la valeur brute
        float panLeft = mySynth->getPan(i, 0);
        state.setProperty("pan_s" + juce::String(i), panLeft - 0.5, nullptr);
    }
    
    // Sauvegarder les volumes via le système original (CC7)
    for (int i = 1; i <= 7; i++)
    {
        state.setProperty("volume_s" + juce::String(i), myMidiInHandler->getVolumeFinal(i), nullptr);
    }
    
    // Sauvegarder les master volumes (CC70)
    for (int i = 1; i <= 7; i++)
    {
        state.setProperty("master_volume_s" + juce::String(i), mySynth->getMasterVolume(i), nullptr);
    }
    
    // Sauvegarder les paramètres de reverb
    state.setProperty("reverb_enabled", mySynth->isReverbEnabled(), nullptr);
    state.setProperty("reverb_roomsize", mySynth->reverb->getroomsize(), nullptr);
    state.setProperty("reverb_wet", mySynth->reverb->getwet(), nullptr);
    state.setProperty("reverb_damp", mySynth->reverb->getdamp(), nullptr);
    state.setProperty("reverb_width", mySynth->reverb->getwidth(), nullptr);
    state.setProperty("reverb_highpass", mySynth->getReverbHighpass(), nullptr);
    state.setProperty("reverb_lowpass", mySynth->getReverbLowpass(), nullptr);
    
    // Convertir en XML et sauvegarder
    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void SirenePlugAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restaurer les paramètres depuis les données sauvegardées
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName("MixerState"))
        {
            juce::ValueTree state = juce::ValueTree::fromXml(*xmlState);
            
            // Restaurer les pans
            for (int i = 1; i <= 7; i++)
            {
                if (state.hasProperty("pan_s" + juce::String(i)))
                {
                    float pan = state.getProperty("pan_s" + juce::String(i));
                    mySynth->setPan(i, pan);
                }
            }
            
            // Restaurer les volumes via le système original (CC7)
            for (int i = 1; i <= 7; i++)
            {
                if (state.hasProperty("volume_s" + juce::String(i)))
                {
                    float volume = state.getProperty("volume_s" + juce::String(i));
                    myMidiInHandler->setVolumeFinal(i, volume);
                }
            }
            
            // Restaurer les master volumes (CC70)
            for (int i = 1; i <= 7; i++)
            {
                if (state.hasProperty("master_volume_s" + juce::String(i)))
                {
                    float masterVol = state.getProperty("master_volume_s" + juce::String(i));
                    mySynth->setMasterVolume(i, masterVol);
                }
            }
            
            // Restaurer les paramètres de reverb
            if (state.hasProperty("reverb_enabled"))
            {
                bool enabled = state.getProperty("reverb_enabled");
                mySynth->setReverbEnabled(enabled);
            }
            
            if (state.hasProperty("reverb_roomsize"))
            {
                float roomsize = state.getProperty("reverb_roomsize");
                mySynth->reverb->setroomsize(roomsize);
            }
            
            if (state.hasProperty("reverb_wet"))
            {
                float wet = state.getProperty("reverb_wet");
                mySynth->reverb->setwet(wet);
            }
            
            if (state.hasProperty("reverb_damp"))
            {
                float damp = state.getProperty("reverb_damp");
                mySynth->reverb->setdamp(damp);
            }
            
            if (state.hasProperty("reverb_width"))
            {
                float width = state.getProperty("reverb_width");
                mySynth->reverb->setwidth(width);
            }
            
            if (state.hasProperty("reverb_highpass"))
            {
                float hpf = state.getProperty("reverb_highpass");
                mySynth->setReverbHighpass(hpf);
            }
            
            if (state.hasProperty("reverb_lowpass"))
            {
                float lpf = state.getProperty("reverb_lowpass");
                mySynth->setReverbLowpass(lpf);
            }
        }
    }
}



int* SirenePlugAudioProcessor::getIntFromMidiMessage(const void * data, int size)
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

void SirenePlugAudioProcessor::timerCallback()
{
    mySynth->s1->setnote();
    mySynth->s2->setnote();
    mySynth->s3->setnote();
    mySynth->s4->setnote();
    mySynth->s5->setnote();
    mySynth->s6->setnote();
    mySynth->s7->setnote();
}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SirenePlugAudioProcessor();
}
