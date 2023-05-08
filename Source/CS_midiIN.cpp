/*
  ==============================================================================

    CS_midiIN.cpp
    Created: 6 May 2020 10:48:16am
    Author:  guyot

  ==============================================================================
*/

#include "CS_midiIN.h"

#include <iostream>
#include <functional>

using namespace std;

MidiIn::MidiIn(const std::function<void(int,int)> onVelocityChanged,
               const std::function<void(int, int)> onEnginePitchChanged
               ) : onVelocityChanged(onVelocityChanged)
    , onEngineSpeedChanged(onEnginePitchChanged)

{
    for (int i =0; i<17; i++)
    {
        ChangevolumegeneralCh[i]=1.0;
        ControlCh[7][i]=127.;
        ControlCh[12][i]=127.;
        ControlCh[13][i]=127.;
        ControlCh[6][i]=60;
        volumefinalCh[i]=500.;
        MSBCh[i]=64;
        isEnVeilleCh[i]=1;
        ancienVeloCh[i]=-1;
        AncienVolFinalCh[i]=-1;
        veloFinal[i]=500;
    }
    isEnVeilleCh[8]=0;
    velociteCh[8]=127;
}

MidiIn::~MidiIn(){
}



void MidiIn::timerAudio(){
    if(isWithSynth){
        for (int Ch=1; Ch<9; Ch++) {
            sendVariaCh(Ch);
            if (isRampeCh[Ch]) {
                createRampeCh(Ch);
            }
            if (isReleaseCh[Ch]) {
                createReleaseCh(Ch);
            }
            if (isAttacVibrato[Ch] && countTimerAudio==0 ) {
                incrementeVibrato(Ch);
            }
        }
        countTimerAudio++;
        if (countTimerAudio>=9)countTimerAudio=0;
    }
}

void MidiIn::JouerClic(int value){
    switch (value%2) {
        case 0:
            //[appDelegate.MonSynth jouerclic1]; //todo
            break;
        case 1:
           //[appDelegate.MonSynth jouerclic2]; //todo
            break;
        default:
            break;
    }
}


void MidiIn::handleMIDIMessage2(int Ch, int value1, int value2){
    std::cout << "Message MIDI reçu: " << Ch << value1 << value2 << std::endl;
    if (Ch >= 144 && Ch < 160 ) {
        if (value2 != 0) {
            if(value2==200) {RealTimeStartNote(Ch-143, value1, 0);}
            else {RealTimeStartNote(Ch-143, value1, value2);}

        }else {
            RealTimeStopNote(Ch-143, value1);
        }
    }
    else if (Ch >= 128 && Ch < 144 ) {
        RealTimeStopNote(Ch-127, value1);
    }
    else if (Ch >= 176 && Ch < 192) {
        HandleControlChange(Ch - 175, value1, value2);
    }else if (Ch >=224 && Ch <240){
        HandlePitchWheel(Ch-223, value1, value2);
    }
}

void MidiIn::RealTimeStartNote(int Ch, int value1, int value2){
    std::cout << "RealTimeStartNote: " << Ch << "-" << value1 << "-" << value2 << std::endl;
    if (Ch >=1 && Ch <8) {
        if(Ch ==1)countvibra = 0;
        if((ControlCh[1][Ch] != 0 && ControlCh[9][Ch] != 0 && ControlCh[11][Ch] != 0 && value2>0 && velociteCh[Ch]==0) ||(ControlCh[1][Ch] != 0 && ControlCh[9][Ch] != 0 && ControlCh[11][Ch] != 0 && value2>0 && value1 !=noteonCh[Ch] ) ){
            Control1FinalCh[Ch]=0;
            isAttacVibrato[Ch]=1;
        }
        noteonCh[Ch] = value1;
        velociteCh[Ch] = value2;
        noteonfinalCh[Ch] = ((noteonCh[Ch]) + pitchbendCh[Ch])  ;
        volumefinalCh[Ch] =(velociteCh[Ch] *(ControlCh[7][Ch]/127.0))*(500./127.) ;
        if(volumefinalCh[Ch] < 0.0)volumefinalCh[Ch]=0.0;
        if(volumefinalCh[Ch] > 500.0)volumefinalCh[Ch]=500.0;
        tourmoteurCh[Ch] = tabledecorresponcanceMidinote(noteonfinalCh[Ch], Ch);
        sendVariaCh(Ch);
        sendVolCh((int) (volumefinalCh[Ch]*ChangevolumegeneralCh[Ch]), Ch);
    }else if(Ch==10){
        if(value1 > 1 && value2 > 1) JouerClic(value1);
    }
}

void MidiIn::RealTimeStopNote(int Ch, int note){
    if (Ch < 8) {
        if(note == noteonCh[Ch])
        {
            sendVariaCh(Ch);
            sendVolCh(0, Ch);
            velociteCh[Ch] = 0.0;
            volumefinalCh[Ch]=0.0;

        }
    }
}

void MidiIn::HandleControlChange(int Ch, int value1, int value2){

    if (Ch < 9) {
        switch (value1)
        {
            case 1 :
                ControlCh[1][Ch] = value2 ;
                if(ControlCh[11][Ch]==0)Control1FinalCh[Ch] =ControlCh[1][Ch];
                if(ControlCh[1][Ch]==0 && isAttacVibrato[Ch]==1){
                    isAttacVibrato[Ch]=0;
                }
                break;
            case 5 :
                ControlCh[5][Ch] =value2;
                break;
            case 6 :
                ControlCh[6][Ch] =value2;
                break;
            case 7 :
                ControlCh[7][Ch] = value2 ;
                volumefinalCh[Ch] =(velociteCh[Ch] *(ControlCh[7][Ch]/127.0))*(500./127.) ;
                if(volumefinalCh[Ch] < 0.0)volumefinalCh[Ch]=0.0;
                if(volumefinalCh[Ch] > 500.0)volumefinalCh[Ch]=500.0;
                sendVolCh((int)(volumefinalCh[Ch]*ChangevolumegeneralCh[Ch]), Ch);
                break;
            case 9 :
                ControlCh[9][Ch] = value2 ;
                if(ControlCh[9][Ch] < 0.0)ControlCh[9][Ch]=0.0;
                if(ControlCh[9][Ch] > 127.0)ControlCh[9][Ch]=127.0;
                if(ControlCh[9][Ch]==0 && isAttacVibrato[Ch]==1){
                    isAttacVibrato[Ch]=0;
                }
                break;
            case 11 :
                ControlCh[11][Ch] = value2 ;
                if(ControlCh[11][Ch]==0 && isAttacVibrato[Ch]==1){
                    isAttacVibrato[Ch]=0;

                }
                break;
            case 15 :
                ControlCh[15][Ch] = value2 ;
                if(value2==0)vartremoloCh[Ch]=0;
                break;
            case 72 :
                ControlCh[72][Ch] = value2;
                break;
            case 73 :
                ControlCh[73][Ch] = value2;
                break;
            case 92 :
                ControlCh[92][Ch]= value2 ;
                break;
            default:
                break;
        }
    }
}

void MidiIn::HandlePitchWheel(int Ch, int value1, int value2){
    if (Ch < 8) {
        pitchbendCh[Ch] = ((value2 << 7) | value1);
        pitchbendCh[Ch] = (float)((pitchbendCh[Ch]) - 8192) / 8192.;
        noteonfinalCh[Ch] = (noteonCh[Ch] + pitchbendCh[Ch] )  ;
        tourmoteurCh[Ch] = tabledecorresponcanceMidinote(noteonfinalCh[Ch], Ch);
    }
}

float MidiIn::tabledecorresponcanceMidinote(float note, int Ch){
    float Midinote=note;
    float tourmoteur=0.;
    float multiplicateur=0.;
    if (Ch ==1 || Ch==2) {
        multiplicateur=5.;
    }else if (Ch ==3 || Ch==5 || Ch==6 || Ch==7) {
        multiplicateur=7.5;
    }else if (Ch ==4){
        multiplicateur=(20./3.);
    }
    if(Midinote < 0.)Midinote = 0.;
    float freq2 = 440. * powf(2., (Midinote-81.)/12.);
    if(freq2 > 8.)tourmoteur= freq2 * multiplicateur;// DO 24
    return tourmoteur;
}

void MidiIn::sendVariaCh(int Ch){

        float vibrato=0.;
        if ((varvfoCh[Ch] <=628) && (ControlCh[9][Ch]!=0) && (ControlCh[1][Ch]!=0))                // <=valeur 2 * PI (pour le calcul du sinus
        {                                                       // et frequence de modulation diffÈrent de 0
            varvfoCh[Ch]=varvfoCh[Ch]+ incrementationVibrato*ControlCh[9][Ch];  // valeur frÈquence = f*10  12,7Hz = 127 pÈriode = 1/frÈq
            vibrato=(((tourmoteurCh[Ch]*constescursion*Control1FinalCh[Ch] )/12700.)*sin(varvfoCh[Ch]/100.));
        }
        else
        {
            varvfoCh[Ch]=0;
            vibrato=0.;
        }
        if ((vartremoloCh[Ch] <=628)&&(ControlCh[15][Ch]!=0)&&(ControlCh[92][Ch]!=0))                 // <=valeur 2 * PI (pour le calcul du sinus
        {                                                          //  et frequence de tremolo diffÈrent de 0
            vartremoloCh[Ch]=vartremoloCh[Ch]+incrementationVibrato*ControlCh[15][Ch]; // valeur frÈquence = f*10  12,7Hz = 127 pÈriode = 1/frÈq
        }
        else
        {
            vartremoloCh[Ch]=0;
        }
        if (ControlCh[15][Ch]!=0 && ControlCh[92][Ch]!=0 && isReleaseCh[Ch]==0 && isRampeCh[Ch]==0 && !isEnVeilleCh[Ch]) {
            int volume=(int)(volumefinalCh[Ch]*ChangevolumegeneralCh[Ch]);
            tremoloCh[Ch] =volume-(((volume*sin(vartremoloCh[Ch]/100.))/(256./ControlCh[92][Ch]))+(volume/(256./ControlCh[92][Ch])));
            sendVolCh(volume, Ch);
        }
        //***************************** Portamento*****************************
        if (ControlCh[5][Ch]==0.0)vitesseCh[Ch]=tourmoteurCh[Ch];
        else {
            float nbr=((ControlCh[5][Ch]/127.)/5.)+0.80;
            vitesseCh[Ch]=(((1.-nbr) * tourmoteurCh[Ch]) + (nbr*vitesseCh[Ch]));
            //***** end Portamento
        }
        if(isWithSynth){
            onEngineSpeedChanged(Ch, vitesseCh[Ch]+vibrato);
        }
}

void MidiIn::sendVolCh(int message, int Ch){
    if( (ControlCh[73][Ch] > 0.0) && (message >=2) && (ancienVeloCh[Ch]<=1  )){// ATTAC
        if(isRampeCh[Ch]==1){
            isRampeCh[Ch]=0;
            countcreateattac[Ch]--;
        }
        if(isReleaseCh[Ch]==1){
            isReleaseCh[Ch]=0;
            countcreaterelease[Ch]--;
        }
        countcreateattac[Ch]++;
        if (countcreateattac[Ch]==1) {
            isRampeCh[Ch]=1;
        }else countcreateattac[Ch]--;

    }
    else if( (ControlCh[72][Ch] > 0.0) && (message <=1) && (ancienVeloCh[Ch] >=2)){//Release
        if(isReleaseCh[Ch]==1){
            isReleaseCh[Ch]=0;
            countcreaterelease[Ch]--;
        }
        if(isRampeCh[Ch]){
            isRampeCh[Ch]=0;
            countcreateattac[Ch]--;
        }
        isReleaseCh[Ch]=1;
    }
    else  {
        if(isRampeCh[Ch]==1 && (message<=1 ) ){
            isRampeCh[Ch]=0;
            countcreateattac[Ch]--;
        }
        if(isReleaseCh[Ch]==1 && message>1  ){
            isReleaseCh[Ch]=0;
            countcreaterelease[Ch]--;
        }
        if((isRampeCh[Ch]==0) && (isReleaseCh[Ch]==0) ){
            if (ControlCh[15][Ch]>0. && ControlCh[92][Ch]>0.){
                vitesseClapetCh[Ch]=veloFinal[Ch]=(int)tremoloCh[Ch];
            }
            else vitesseClapetCh[Ch]=veloFinal[Ch]=message;

            if(isWithSynth && Ch !=8){
                onVelocityChanged(Ch, veloFinal[Ch]);
            }
        }

    }
    ancienVeloCh[Ch]=message;
}


void MidiIn::createReleaseCh(int Ch){
    float nbr=128-ControlCh[72][Ch];
    if (vitesseClapetCh[Ch] >=250)nbr=nbr/7.62;
    else if (vitesseClapetCh[Ch] < 250 && vitesseClapetCh[Ch] >= 200)nbr=nbr/10.;
    else if (vitesseClapetCh[Ch] < 200 && vitesseClapetCh[Ch] >= 150)nbr=nbr/15.;
    else if (vitesseClapetCh[Ch] < 150 && vitesseClapetCh[Ch] >= 100)nbr=nbr/20;
    else if (vitesseClapetCh[Ch] < 100 && vitesseClapetCh[Ch] >= 50)nbr=nbr/25.;
    else if (vitesseClapetCh[Ch] < 50 )nbr=nbr/30.;
    vitesseClapetCh[Ch]=vitesseClapetCh[Ch]-nbr;
    int around=(int)vitesseClapetCh[Ch];
    if (around <= 1 ) {
        if(isReleaseCh[Ch]==1){
            isReleaseCh[Ch]=0;
        }
        countcreaterelease[Ch]--;
        tremoloCh[Ch]=veloFinal[Ch]=around=0;
    }else if (ControlCh[15][Ch]!=0. && ControlCh[92][Ch]!=0.) {
        tremoloCh[Ch] =around-(((around*sin(vartremoloCh[Ch]/100.))/(256./ControlCh[92][Ch]))+(around/(256./ControlCh[92][Ch])));
        veloFinal[Ch]=(int)tremoloCh[Ch];
    }else veloFinal[Ch]=around;
    if(isWithSynth && Ch !=8){
        onVelocityChanged(Ch, veloFinal[Ch]);
    }
}

void MidiIn::createRampeCh(int Ch){
    int vitesseVoulue=volumefinalCh[Ch]*ChangevolumegeneralCh[Ch];
    float nbr=(128-ControlCh[73][Ch])/7.62;
    vitesseClapetCh[Ch]=vitesseClapetCh[Ch]+nbr;
    int around=(int)vitesseClapetCh[Ch];
    if (around >= vitesseVoulue ) {
        if(isRampeCh[Ch]==1){
            isRampeCh[Ch]=0;
            countcreateattac[Ch]--;
        }
        veloFinal[Ch]=vitesseClapetCh[Ch]=around=vitesseVoulue;
    }
    if (ControlCh[15][Ch]!=0. && ControlCh[92][Ch]!=0.) {
        tremoloCh[Ch] =around-(((around*sin(vartremoloCh[Ch]/100.))/(256./ControlCh[92][Ch]))+(around/(256./ControlCh[92][Ch])));
        veloFinal[Ch]=(int)tremoloCh[Ch];
    }else  veloFinal[Ch]=around;

    if(isWithSynth && Ch !=8){
        onVelocityChanged(Ch, veloFinal[Ch]);
    }
}

void MidiIn::isWithSound(bool is){
    isWithSynth=is;
}

void MidiIn::changingvolumeclic(int VolumeClic){
    VolumeDuClic=VolumeClic;
}



void MidiIn::incrementeVibrato(int Ch){
    if (Control1FinalCh[Ch] < ControlCh[1][Ch]) {
        Control1FinalCh[Ch]=Control1FinalCh[Ch]+(ControlCh[11][Ch]/12.7);
    }else {
        Control1FinalCh[Ch]=ControlCh[1][Ch];
        isAttacVibrato[Ch]=0;
    }
}

void MidiIn::definiMuteEthernet(bool ismuted, int Ch){
    if (ismuted) {
        tourmoteurCh[Ch] =0.0;
        noteonfinalCh[Ch]=0.0;
        volumefinalCh[Ch]=0.0;
        sendVariaCh(Ch);
        sendVolCh(0, Ch);
    }
    isMuteEthernetCh[Ch]=ismuted;
}

void MidiIn::STOffVariateurCh(int Ch){
    isEnVeilleCh[Ch]=1;
}

void MidiIn::STOnVariateurCh(int Ch){
    isEnVeilleCh[Ch]=0;
}

void MidiIn::resetSireneCh(int Ch){

    std::cout << "Reset: "<< Ch << std::endl;

    noteonfinalCh[Ch]=0.0;
    ///////////////////////////////////////////////////****** Ferme les volets

    AncienVolFinalCh[Ch]=-1;
    ControlCh[1][Ch]=0;
    ControlCh[5][Ch]=0;
    ControlCh[9][Ch]=0;
    ControlCh[11][Ch]=0;
    ControlCh[15][Ch]=0;
    ControlCh[17][Ch]=0;
    ControlCh[18][Ch]=0;
    ControlCh[92][Ch]=0;
    ControlCh[72][Ch]=0;
    ControlCh[73][Ch]=0;
    LSBCh[Ch]=0;
    MSBCh[Ch]=64;
    pitchbendCh[Ch]=0.;
    noteonCh[Ch]=0.;
    velociteCh[Ch]=0;
    tourmoteurCh[Ch] =0.0;
    noteonfinalCh[Ch]=0.0;
    volumefinalCh[Ch]=0.0;
    ControlCh[12][Ch]=127.;
    ControlCh[13][Ch]=127.;
    Control1FinalCh[16]=0.;
    varvfoCh[Ch]=0;
    vartremoloCh[Ch]=0;
    vitesseCh[Ch]=0;
    tremoloCh[Ch]=0;


    isAttacVibrato[Ch]=0;

    if (isRampeCh[Ch]==1){
        isRampeCh[Ch]=0;
        countcreateattac[Ch]--;
    }
    if (isReleaseCh[Ch]==1){
        isReleaseCh[Ch]=0;
        countcreaterelease[Ch]--;
        isReleaseCh[Ch]=0;
    }
    
    ControlCh[7][Ch]=127;
    veloFinal[Ch]=500;
    volumefinalCh[Ch]=500.;
    vitesseClapetCh[Ch]=0;
    ancienVeloCh[Ch]=-1;
    AncienVolFinalCh[Ch]=-1;
}
