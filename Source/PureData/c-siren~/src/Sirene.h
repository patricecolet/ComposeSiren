/*
  Standalone Sirene core for Pure Data external.
  Extracted from ComposeSiren common engine without JUCE dependencies.
*/

#pragma once

#include <cmath>
#include <string>

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
};

class Sirene {
public:
  Sirene(const std::string& str, const std::string& dataFolderPath);
  ~Sirene();

  void setMidicent(int note);
  void setnoteFromExt(int note);
  void setnote();
  SireneSpeedSlideState oujesuis();
  void changeQualite(int qualt);
  void set16ou8Bit(bool is);
  void setVelocite(int velo);
  void setSampleRate(double newSampleRate);
  void setisCrossFade(int is);

  void readDataFromBinaryFile(
    std::string dataFilePath,
    std::string tabAmpFile,
    std::string tabFreqFile,
    std::string dureTabFile,
    std::string vectorIntervalFile
  );

  inline float calculwave() {
    isChangementdenote = false;
    float wavefinal = 0.0f;

    if (noteInf != ancienNoteVoulue) {
      isChangementdenote = true;
      ancienNoteVoulue = noteInf;
    }

    if (countKInf == static_cast<int>(dureTabs[noteInf][0])) {
      countP[noteInf]++;
      if (countP[noteInf] == static_cast<int>(dureTabs[noteInf][1])) {
        countP[noteInf] = 0;
      }
      countKInf = 0;
    }
    countKInf++;

    if (countKSup == static_cast<int>(dureTabs[noteSup][0])) {
      countP[noteSup]++;
      if (countP[noteSup] == static_cast<int>(dureTabs[noteSup][1])) {
        countP[noteSup] = 0;
      }
      countKSup = 0;
    }
    countKSup++;

    if (ampvouluz < ampvoulu) ampvouluz += vitesseClape;
    if (ampvouluz > ampvoulu) ampvouluz -= vitesseClape;

    float waveInf = 0.0f;
    eloignementfreq = ((noteSup) * 100) - midiCentVoulue;
    count8bit = !count8bit;

    for (int i = 0; i < qualite; i++) {
      if (is16Bit || isChangementdenote || count8bit) {
        if (isCrossfade) {
          phaseInf[i] += (
            (tabFreq[noteInf][countP[noteInf]][i] * pitchSchift[noteInf] * eloignementfreq / 100.0f)
          ) + (
            (tabFreq[noteSup][countP[noteSup]][i] * pitchSchift[noteSup] * (100 - eloignementfreq) / 100.0f)
          );

          amp[i] = (
            tabAmp[noteInf][countP[noteInf]][i] * eloignementfreq / 100.0f
          ) + (
            tabAmp[noteSup][countP[noteSup]][i] * (100 - eloignementfreq) / 100.0f
          );
        } else {
          amp[i] = tabAmp[noteInf][countP[noteInf]][i];
          phaseInf[i] += tabFreq[noteInf][countP[noteInf]][i] * pitchSchift[noteInf];
        }

        ampz[i] = 0.001f * amp[i] + 0.999f * ampz[i];
        waveInf += std::sin(phaseInf[i]) * ampz[i];
        anciennewaveInf = waveInf;
        if (phaseInf[i] == 180.0) {
          phaseInf[i] = 0.0;
        }
      } else {
        phaseInf[i] += tabFreq[noteInf][countP[noteInf]][i] * pitchSchift[noteInf];
      }
    }

    if (is16Bit) wavefinal = waveInf * ampvouluz;
    else wavefinal = anciennewaveInf * ampvouluz;

    if (noteEncour <= noteMin * 100) {
      wavefinal = 0.0f;
    }

    return wavefinal;
  }

private:
  std::string name;
  int noteMidiCentMax;
  int noteMin;
  int pourcentClapetOff;
  int coeffPicolo;
  float inertiaFactorTweak;

  double sampleRate;
  double deuxPieSampleRate;

  float tabAmp[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
  float tabFreq[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
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
  float pitchSchift[NOMBRE_DE_NOTE] = { 0 };
  float anciennewaveInf = 0.0f;
  int eloignementfreq = 0;
  int noteInf = 0;
  int noteSup = 0;
  float ampvoulu = 1.0f;
  float ampvouluz = 1.0f;
  bool isChangementdenote = false;
  float ampz[MAX_Partiel] = { 0 };
  float amp[MAX_Partiel] = { 0 };

  float ampMax = 1.0f;
  bool is16Bit = false;
  int noteVoulueAvantSlide = 0;
  float noteEncour = 0;
  int interDepart = 0;
  bool isCrossfade = false;

  int computeInertiaBias(SireneSpeedSlideState ouJeSuis);
};
