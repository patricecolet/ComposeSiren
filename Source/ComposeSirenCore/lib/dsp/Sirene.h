/*
  ==============================================================================

    S1.h
    Created: 6 May 2020 10:49:33am
    Author:  guyot

  ==============================================================================
*/

#pragma once

#include <iostream>

#include "lib/definitions/sirenProperties.h"

//
//  S1.h
//  FourrerN
//
//  Created by Maria Isabella Hallberg Møller on 25/07/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#if defined (_MSC_VER)
// code specific to Visual Studio compiler
#define _USE_MATH_DEFINES
#include <math.h>
// without those, no M_PI.
#else
// regular stuff for other compilers
#include <math.h>
#endif

#define MAX_Partiel 200
#define NOMBRE_DE_NOTE 80
#define MAX_TAB 1000

enum SireneSpeedSlideState {
    Montant         = 0,
    Descandant      = 1,
    TonUpBefore     = 2,
    DemiUpBefore    = 3,
    QuartUpBefore   = 4,
    Boucle          = 5,
    QuartDownAfter  = 6,
    QuartDownBefore = 7,
    QuartUpAfter    = 8,
    jesuisrest      = 9,
} ;

class Sirene {
public:
  // Sirene(const sirenCategory c = Alto, const std::string& dataFolderPath);
  // Sirene(const std::string& str, const std::string& dataFolderPath);
  Sirene(sirenId id, const std::string& dataFolderPath);
  ~Sirene();

private:
  std::shared_ptr<sirenData> data;
  // Pat added ------------------
  // std::string name;
  int noteMidiCentMax;
  int noteMin;
  int pourcentClapetOff;
  
  int coeffPicolo;
  float inertiaFactorTweak;

  double sampleRate;
  double deuxPieSampleRate;

public:
  void setnote();
  void setVelocite(int velo);
  void setnoteFromExt(int note);
  SireneSpeedSlideState oujesuis();
  void changeQualite(int qualt);
  void set16ou8Bit(bool is);
  void setSampleRate(double newSampleRate);
  void setisCrossFade(int is);

  void readDataFromBinaryFile(
    const std::string& dataFilePath,
    const std::string& tabAmpFile,
    const std::string& tabFreqFile,
    const std::string& dureTabFile,
    const std::string& vectorIntervalFile
  );

  float getCurrentPitch();

private:
    void setMidicent(int note);

public:
  float  calculwave() {
    isChangementdenote = false;
    float wavefinal = 0.;

    if (noteInf != ancienNoteVoulue) {
      isChangementdenote = true;
      ancienNoteVoulue = noteInf;
    }

    if (countKInf == static_cast<int>(dureTabs[noteInf][0])) {
      countP[noteInf]++;
      if (countP[noteInf] == static_cast<int>(dureTabs[noteInf][1]))
        countP[noteInf] = 0;
      countKInf = 0;
    }
    countKInf++;
    
    if (countKSup == static_cast<int>(dureTabs[noteSup][0])) {
      countP[noteSup]++;
      if (countP[noteSup] == static_cast<int>(dureTabs[noteSup][1]))
        countP[noteSup] = 0;
      countKSup = 0;
    }
    countKSup++;

    if (ampvouluz < ampvoulu) ampvouluz += vitesseClape;
    if (ampvouluz > ampvoulu) ampvouluz -= vitesseClape;

    float waveInf = 0.;
    eloignementfreq = ((noteSup) * 100) - midiCentVoulue;
    count8bit = !count8bit;

    for (int i = 0; i < qualite; i++) {
      if (is16Bit || isChangementdenote || count8bit) {
        if (isCrossfade) {
          phaseInf[i] += (
            tabFreq[noteInf][countP[noteInf]][i] *
            pitchSchift[noteInf] *
            eloignementfreq / 100.
          ) + (
            tabFreq[noteSup][countP[noteSup]][i] *
            pitchSchift[noteSup] *
            (100 - eloignementfreq) / 100.
          );

          amp[i] = (
            tabAmp[noteInf][countP[noteInf]][i] *
            eloignementfreq / 100.
          ) + (
            tabAmp[noteSup][countP[noteSup]][i] *
            (100 - eloignementfreq) / 100.
          );
        } else {
          amp[i] = tabAmp[noteInf][countP[noteInf]][i];
          phaseInf[i] += (tabFreq[noteInf][countP[noteInf]][i] * pitchSchift[noteInf]);
        }

        ampz[i] = 0.001 * amp[i] + 0.999 * ampz[i] ;
        waveInf += sin(phaseInf[i]) * ampz[i];
        anciennewaveInf = waveInf;
        if (phaseInf[i] == 180.)
          phaseInf[i] = 0.;

      } else {
        phaseInf[i] += (tabFreq[noteInf][countP[noteInf]][i] * pitchSchift[noteInf]);
      }
    }

    if (is16Bit) wavefinal = waveInf * ampvouluz;
    else wavefinal = anciennewaveInf * ampvouluz;
    if (noteEncour <= noteMin * 100) wavefinal = 0.;

    return wavefinal;
  }

private:
  // conso memoire tabAmp / tabFreq :
  // sizeof(float) * NOMBRE_DE_NOTE * MAX_TAB * MAX_Partiel = 32 * 80 * 1000 * 200
  // 480 millions de bits = 60MB chacun
  // pour dureTabs on a 32 * 240 / 8 = 960 B ~1kB
  // et vectorInterval 32 * 392 / 8 = 1568 B ~1kB
  // soit ~120MB par instance de sirene et ~240MB le temps de swapper entre deux modeles
  // NB : in C++ 3D arrays memory layout is layer major order (then row, then column)
  // like this : myArray[columns][rows][layers] (outermost dimension major order in general)
  float tabAmp[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
  float tabFreq[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
  // dureTabs : 0 = dureTab en samples, 1 = nombreMax de Tab, 2 = FreqMoyenne
  float dureTabs[NOMBRE_DE_NOTE][3];
  float vectorInterval[392];

  bool count8bit = true;
  double vitesseClape = 0.0002;
  int countKInf = 0;
  int countKSup = 0;
  int midiCentVoulue = 0;
  int ancienNoteVoulue = 0;
  int qualite = 30;
  double phaseInf[MAX_Partiel] = { 0 };
  double phaseSup[MAX_Partiel] = { 0 };
  int countP[NOMBRE_DE_NOTE] = { 0 };
  float pitchSchift[NOMBRE_DE_NOTE] = {0};
  float anciennewaveInf = 0.;
  int eloignementfreq = 0;
  int noteInf = 0;
  int noteSup = 0;
  float ampvoulu = 1.;
  float ampvouluz = 1.;
  bool isChangementdenote = false;
  float ampz[MAX_Partiel] = { 0 };
  float amp[MAX_Partiel] = { 0 };

  float ampMax = 1.;
  bool is16Bit = false;
  int noteVoulueAvantSlide = 0; // gauthier: deterministically init noteVoulueAvantSlide to 0
  float noteEncour = 0; // gauthier: deterministically init noteEncour to 0
  int interDepart = 0;  // gauthier: deterministically init interDepart to 0
  bool isCrossfade = false; // gauthier: deterministically init isCrossfade to false

  int computeInertiaBias(SireneSpeedSlideState ouJeSuis);

  float computeInertiaFactor(float cents){
      float octaveRatio = cents / 1200;
      if (octaveRatio == 0. || octaveRatio < 0.01) { return 1.; }
      else {
          return 1. / octaveRatio;
      }
  }
};
