/*
  ==============================================================================

    CS_midiIN.h
    Created: 6 May 2020 10:48:30am
    Author:  guyot

  ==============================================================================
*/

#pragma once


#include <math.h>
#include "synth.h"
#include <functional>


#define constescursion 10					// valeur maxi de l'amplitude VFO en % de la vitesse correspondant ‡ une note


class MidiIn
{
public:
    MidiIn(
           const std::function<void(int,int)> onVelocityChanged,
           const std::function<void(int, int)> onEnginePitchChanged
           );
  ~MidiIn();

  void definiMuteEthernet(bool ismuted, int Ch);
  void handleMIDIMessage2(int Ch, int value1, int value2);

  void RealTimeStartNote(int Ch, int value1, int value2);
  void RealTimeStopNote(int Ch, int note);
  void HandleControlChange(int Ch, int value1, int value2);
  void HandlePitchWheel(int Ch, int value1, int value2);

  float tabledecorresponcanceMidinote(float note, int Ch);

  void sendVolCh(int message, int Ch);
  void sendVariaCh(int Ch);



  void JouerClic(int value);
  void isWithSound(bool is);
  void changingvolumeclic(int VolumeClic);


  void STOffVariateurCh(int Ch);
  void STOnVariateurCh(int Ch);

  void createRampeCh(int Ch);
  void createReleaseCh(int Ch);

  void resetSireneCh(int Ch);

  void incrementeVibrato(int Ch);

  void timerAudio();
  void sirenium_in(unsigned char *buf);

  //Synth* mySynth;

private:

  float ChangevolumegeneralCh[17];

    float noteonCh[17] = {0} ;
    float velociteCh[17] = {0};
    float pitchbendCh[17] = {0};
    float ControlCh[127][17] = {0};
    float Control1FinalCh[17] = {0};
    float noteonfinalCh[17]  = {0};
  float volumefinalCh[17] ;
    float tourmoteurCh[17] = {0};
    int LSBCh[17] = {0};
  int MSBCh[17];
    float varvfoCh[17] = {0};
    float vartremoloCh[17] = {0};
  int isEnVeilleCh[17];
    float vitesseCh[17] = {0};
    float tremoloCh[17] = {0};
  int veloFinal[17];
  float incrementationVibrato=(512./44100.)/0.025;


  bool isWithSoundON;
  int VolumeDuClic = 100;

    float vitesseClapetCh[17] = {0};
  int ancienVeloCh[17];

  bool isWithSynth = true;

  int AncienVolFinalCh[17];
    int isMuteEthernetCh[17] = {0};

    int countcreaterelease[17] = {0};
    int countcreateattac[17] = {0};

    int isAttacVibrato[9] = {0};
  int countTimerAudio = 0;
  int isRampeCh[9];
  int isReleaseCh[9];

  int countvibra = 0;
  int pitch_bend;

  //Synth* mySynth;

    const std::function<void(int,int)> onVelocityChanged;
    
    const std::function<void(int, int)> onEngineSpeedChanged;
};
