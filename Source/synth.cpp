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

Synth::Synth(){

    //mIntervalFromAudio=512/44100;
    //ICI TOUT LE TRUC DE OBJECTIVE C
    //fOnOffReverbe=true;
    
    // left channel
    PanS1=0.75;
    PanS2=0.25;
    PanS3=0.6;
    PanS4=0.4;
    PanS5=0.9;
    PanS6=0.1;
    PanS7=0.45;
    
    //muteS1=false;
    isWithSynthe=true;
    isWithClic=false;
    //volumeSynth=1.;
    //volumeClic=1.;
    //SizeBufClic1 = sizeof(clic1);
    //SizeBufClic2 = sizeof(clic2);
    //ptrClic1=(short*)clic1;
    //ptrClic2=(short*)clic2;
    //countWherWeAreClic1=0;
    //countWherWeAreClic2=0;
    //jouerclic1=false;
    //jouerclic2=false;
    //Volsynthz = 1.;
    //VolClicz = 1.;
    WideCoeff=1.5;
    
#if defined (_MSC_VER)
    std::string dataFilePath = "C:\\Program Files\\Common Files\\Mecanique Vivante\\ComposeSiren\\Resources\\";
#else
#if CMS_BUILD_WITH_PROJUCER
    std::string dataFilePath = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getChildFile ("Contents/Resources/").getFullPathName().toStdString() + '/';
#elif CMS_BUILD_WITH_CMAKE
    std::string dataFilePath = "/Library/Audio/Plug-ins/Mecanique Vivante/ComposeSiren/Resources/";
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
}


void Synth::setnote(int sireneNumber, int note)
{
    //std::cout<< "Set note. sirene : " << sireneNumber << ", note : " << note << std::endl;
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


void Synth::setVitesse(int chanal, float vitesse){
    //std::cout<< "Set Vitesse. chanal : " << chanal << ", vitesse : " << vitesse << std::endl;
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
