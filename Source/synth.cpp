/*
  ==============================================================================

    synth.cpp
    Created: 7 May 2020 10:56:27am
    Author:  guyot

  ==============================================================================
*/

#include "synth.h"

#include <JuceHeader.h>
#include <iostream>
#include <vector>

Synth::Synth(){

    // left channel
    PanS1=0.75;
    PanS2=0.25;
    PanS3=0.6;
    PanS4=0.4;
    PanS5=0.9;
    PanS6=0.1;
    PanS7=0.45;
    
    // Volumes indépendants (master) initialisés à 1.0 (100%)
    masterVolumeS1 = 1.0f;
    masterVolumeS2 = 1.0f;
    masterVolumeS3 = 1.0f;
    masterVolumeS4 = 1.0f;
    masterVolumeS5 = 1.0f;
    masterVolumeS6 = 1.0f;
    masterVolumeS7 = 1.0f;
    
    // Initialiser la reverb
    reverb = new mareverbe();
    reverbEnabled = false;
    reverbHighpassFreq = 20.0f;
    reverbLowpassFreq = 20000.0f;
    currentSampleRate = 44100.0; // Sera mis à jour dans setSampleRate
    
    // Initialiser les filtres IIR avec des coefficients par défaut
    reverbHighpassL = std::make_unique<juce::IIRFilter>();
    reverbHighpassR = std::make_unique<juce::IIRFilter>();
    reverbLowpassL = std::make_unique<juce::IIRFilter>();
    reverbLowpassR = std::make_unique<juce::IIRFilter>();
    
    reverbHighpassL->setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, reverbHighpassFreq));
    reverbHighpassR->setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, reverbHighpassFreq));
    reverbLowpassL->setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, reverbLowpassFreq));
    reverbLowpassR->setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, reverbLowpassFreq));
    
    isWithSynthe=true;
    isWithClic=false;
    WideCoeff=1.5;
    
    // Déterminer le chemin des ressources selon le contexte
    std::string dataFilePath;
    
#ifdef JucePlugin_Build_Standalone
    // Pour le standalone, chercher les ressources dans le bundle de l'app
    juce::File resourcesDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                .getParentDirectory()
                                .getChildFile("../Resources");
    if (!resourcesDir.exists()) {
        // Fallback: chercher dans le dossier du projet (pour développement)
        resourcesDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                        .getParentDirectory()
                        .getParentDirectory()
                        .getParentDirectory()
                        .getChildFile("Resources");
    }
    #if !defined(__APPLE__) && !defined(_MSC_VER)
    // Fallback Linux : chercher dans le chemin système
    if (!resourcesDir.exists()) {
        resourcesDir = juce::File("/usr/share/ComposeSiren/Resources");
    }
    #endif
    dataFilePath = resourcesDir.getFullPathName().toStdString() + "/";
    
    // Log du chemin des ressources
    DBG("=== ComposeSiren Resources Path ===");
    DBG("Resources directory: " + resourcesDir.getFullPathName());
    DBG("Directory exists: " + juce::String(resourcesDir.exists() ? "YES" : "NO"));
#else
    // Pour les plugins (AU, VST, etc.), utiliser le chemin d'installation
    #if defined (_MSC_VER)
        // Windows
        dataFilePath = "C:\\Program Files\\Common Files\\Mecanique Vivante\\ComposeSiren\\Resources\\";
    #elif defined (__APPLE__)
        // macOS
        dataFilePath = "/Library/Audio/Plug-ins/Mecanique Vivante/ComposeSiren/Resources/";
    #else
        // Linux
        dataFilePath = "/usr/share/ComposeSiren/Resources/";
    #endif
#endif
    
    s1 = new Sirene("S1", dataFilePath);
    s2 = new Sirene("S2", dataFilePath);
    s3 = new Sirene("S3", dataFilePath);
    s4 = new Sirene("S4", dataFilePath);
    s5 = new Sirene("S5", dataFilePath);
    s6 = new Sirene("S6", dataFilePath);
    s7 = new Sirene("S7", dataFilePath);
}

Synth::~Synth(){
    delete (s1);
    delete (s2);
    delete (s3);
    delete (s4);
    delete (s5);
    delete (s6);
    delete (s7);
    delete (reverb);
}

void Synth::setSampleRate(double newSampleRate) {
    // Propager le sample rate à toutes les sirènes
    s1->setSampleRate(newSampleRate);
    s2->setSampleRate(newSampleRate);
    s3->setSampleRate(newSampleRate);
    s4->setSampleRate(newSampleRate);
    s5->setSampleRate(newSampleRate);
    s6->setSampleRate(newSampleRate);
    s7->setSampleRate(newSampleRate);
    
    // Mettre à jour le sample rate pour les filtres reverb
    currentSampleRate = newSampleRate;
    
    // Recalculer les coefficients des filtres
    if (reverbHighpassL) reverbHighpassL->setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, reverbHighpassFreq));
    if (reverbHighpassR) reverbHighpassR->setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, reverbHighpassFreq));
    if (reverbLowpassL) reverbLowpassL->setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, reverbLowpassFreq));
    if (reverbLowpassR) reverbLowpassR->setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, reverbLowpassFreq));
}


void Synth::setnote(int sireneNumber, int note)
{
    if(isWithSynthe){
        switch (sireneNumber) {
            case 1:s1->setnoteFromExt(note);break;
            case 2:s2->setnoteFromExt(note);break;
            case 3:s3->setnoteFromExt(note);break;
            case 4:s4->setnoteFromExt(note);break;
            case 5:s5->setnoteFromExt(note);break;
            case 6:s6->setnoteFromExt(note);break;
            case 7:s7->setnoteFromExt(note);break;
            case 8: break;
            default:
                break;
        }
    }

}

void Synth::setVelocite(int sireneNumber, int velo){
    if(isWithSynthe){
        switch (sireneNumber) {
            case 1:s1->setVelocite(velo);break;
            case 2:s2->setVelocite(velo);break;
            case 3:s3->setVelocite(velo);break;
            case 4:s4->setVelocite(velo);break;
            case 5:s5->setVelocite(velo);break;
            case 6:s6->setVelocite(velo);break;
            case 7:s7->setVelocite(velo);break;
            case 8: break;
            default:
                break;
        }
    }
}


void Synth::setPan(int sireneNumber, float value){
    if(isWithSynthe){
        switch (sireneNumber) {
            case 1:PanS1=value;  break;
            case 2:PanS2=value;  break;
            case 3:PanS3=value;  break;
            case 4:PanS4=value;  break;
            case 5:PanS5=value;  break;
            case 6:PanS6=value;  break;
            case 7:PanS7=value;  break;
            default:
                break;
        }
    }
}

float Synth::getPan(int sireneNumber, int channel)
{
    // Return the panoramic value according to the sirene number and the channel (left : 0, right :1)
    if(channel){
        // right channel
        switch (sireneNumber) {
            case 1: return 1-PanS1 + 0.5;  break;
            case 2: return 1-PanS2+ 0.5;  break;
            case 3: return 1-PanS3+ 0.5;  break;
            case 4: return 1-PanS4+ 0.5;  break;
            case 5: return 1-PanS5+ 0.5;  break;
            case 6: return 1-PanS6+ 0.5;  break;
            case 7: return 1-PanS7+ 0.5;  break;
            default:  return 0.5;
        }
    }
    else
    {
        // left channel

        switch (sireneNumber) {
            case 1: return PanS1+ 0.5;  break;
            case 2: return PanS2+ 0.5;  break;
            case 3: return PanS3+ 0.5;  break;
            case 4: return PanS4+ 0.5;  break;
            case 5: return PanS5+ 0.5;  break;
            case 6: return PanS6+ 0.5;  break;
            case 7: return PanS7+ 0.5;  break;
            default:  return 0.5;
        
    }
    }
}


void Synth::changeQualite(int qualt){
    if(isWithSynthe){
        s1->changeQualite(qualt);
        s2->changeQualite(qualt);
        s3->changeQualite(qualt);
        s4->changeQualite(qualt);
        s5->changeQualite(qualt);
        s6->changeQualite(qualt);
        s7->changeQualite(qualt);
    }
}

void Synth::setMasterVolume(int sireneNumber, float volume){
    if(volume < 0.0f) volume = 0.0f;
    if(volume > 1.0f) volume = 1.0f;
    
    switch (sireneNumber) {
        case 1: masterVolumeS1 = volume; break;
        case 2: masterVolumeS2 = volume; break;
        case 3: masterVolumeS3 = volume; break;
        case 4: masterVolumeS4 = volume; break;
        case 5: masterVolumeS5 = volume; break;
        case 6: masterVolumeS6 = volume; break;
        case 7: masterVolumeS7 = volume; break;
        default: break;
    }
}

float Synth::getMasterVolume(int sireneNumber){
    switch (sireneNumber) {
        case 1: return masterVolumeS1;
        case 2: return masterVolumeS2;
        case 3: return masterVolumeS3;
        case 4: return masterVolumeS4;
        case 5: return masterVolumeS5;
        case 6: return masterVolumeS6;
        case 7: return masterVolumeS7;
        default: return 1.0f;
    }
}

void Synth::setReverbEnabled(bool enabled){
    reverbEnabled = enabled;
}

bool Synth::isReverbEnabled(){
    return reverbEnabled;
}

void Synth::setReverbHighpass(float freq){
    reverbHighpassFreq = freq;
    // Mettre à jour les coefficients du filtre
    if (reverbHighpassL) reverbHighpassL->setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, freq));
    if (reverbHighpassR) reverbHighpassR->setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, freq));
}

float Synth::getReverbHighpass(){
    return reverbHighpassFreq;
}

void Synth::setReverbLowpass(float freq){
    reverbLowpassFreq = freq;
    // Mettre à jour les coefficients du filtre
    if (reverbLowpassL) reverbLowpassL->setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, freq));
    if (reverbLowpassR) reverbLowpassR->setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, freq));
}

float Synth::getReverbLowpass(){
    return reverbLowpassFreq;
}

void Synth::processReverbWithFilters(float* left, float* right, int numSamples){
    if (!reverbEnabled || numSamples <= 0 || !reverbHighpassL || !reverbHighpassR || !reverbLowpassL || !reverbLowpassR) return;
    
    // Buffers temporaires pour dry et wet
    std::vector<float> dryLeft(numSamples);
    std::vector<float> dryRight(numSamples);
    std::vector<float> wetLeft(numSamples);
    std::vector<float> wetRight(numSamples);
    
    // Sauvegarder le dry (signal original)
    float dryLevel = reverb->getdry();
    float wetLevel = reverb->getwet();
    
    for (int i = 0; i < numSamples; i++)
    {
        dryLeft[i] = left[i] * dryLevel;
        dryRight[i] = right[i] * dryLevel;
        wetLeft[i] = left[i];
        wetRight[i] = right[i];
    }
    
    // 1. Appliquer le highpass sur le wet avant la reverb
    for (int i = 0; i < numSamples; i++)
    {
        wetLeft[i] = reverbHighpassL->processSingleSampleRaw(wetLeft[i]);
        wetRight[i] = reverbHighpassR->processSingleSampleRaw(wetRight[i]);
    }
    
    // 2. Appliquer la reverb sur le wet (avec dry=0 pour éviter le double dry)
    float originalDry = reverb->dry;
    reverb->setdry(0.0f); // Temporairement mettre dry à 0
    reverb->processreplace(wetLeft.data(), wetRight.data(), wetLeft.data(), wetRight.data(), numSamples, 1);
    reverb->dry = originalDry; // Restaurer
    
    // 3. Appliquer le lowpass sur le wet après la reverb
    for (int i = 0; i < numSamples; i++)
    {
        wetLeft[i] = reverbLowpassL->processSingleSampleRaw(wetLeft[i]);
        wetRight[i] = reverbLowpassR->processSingleSampleRaw(wetRight[i]);
    }
    
    // 4. Mixer dry (non filtré) + wet (filtré)
    for (int i = 0; i < numSamples; i++)
    {
        left[i] = dryLeft[i] + wetLeft[i];
        right[i] = dryRight[i] + wetRight[i];
    }
}

void Synth::setVitesse(int chanal, float vitesse){
    if(isWithSynthe){
        int midicent=0;
        switch (chanal) {
            case 1:
                midicent=(int)roundf((69+12.*log2f((vitesse/5.)/440.0))*100.);
                if (midicent<0) midicent=0;
                setnote(1, midicent);
                break;
            case 2:
                midicent=(int)roundf((69+12.*log2f((vitesse/5.)/440.0))*100.);
                if (midicent<0) midicent=0;
                setnote(2, midicent);
                break;
            case 3:
                midicent=(int)roundf((69+12.*log2f((vitesse/7.5)/440.0))*100.);
                if (midicent<0) midicent=0;
                setnote(3, midicent);
                break;
            case 4:
                midicent=(int)roundf((69+12.*log2f((vitesse/(20./3.))/440.0))*100.);
                if (midicent<0) midicent=0;
                setnote(4, midicent);
                break;
            case 5:
                midicent=(int)roundf((69+12.*log2f((vitesse/7.5)/440.0))*100.);
                if (midicent<0) midicent=0;
                setnote(5, midicent);
                break;
            case 6:
                midicent=(int)roundf((69+12.*log2f((vitesse/7.5)/440.0))*100.);
                if (midicent<0) midicent=0;
                setnote(6, midicent);
                break;
            case 7:
                midicent=(int)roundf((69+12.*log2f((vitesse/7.5)/440.0))*100.);
                if (midicent<0) midicent=0;
                setnote(7, midicent);
                break;


            default:
                break;
        }
    }
}


