
#include "Sirene.h"
// #include <JuceHeader.h>

#include <iostream>

// see precision
#include <iomanip>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>

// we need this to ensure file loading will not fail if floats are not 32 bits,
// although it's highly unlikely to happen on macos / windows and common linux distros
#include <climits>

static_assert(CHAR_BIT == 8 && sizeof(float) == 4, "require 8 bits bytes and 32 bits floats");
// or is this enough ?
// static_assert(sizeof(float) * CHAR_BIT == 32, "require 32 bits floats");

Sirene::Sirene(sirenId id, const std::string& dataFilePath) :
data(sirenPropertiesById.at(id)) {
  setSampleRate(44100.0);
  memset(&tabAmp, 0, sizeof(tabAmp));
  memset(&tabFreq, 0, sizeof(tabFreq));
  memset(&dureTabs, 0, sizeof(dureTabs));
  memset(&vectorInterval, 0, sizeof(vectorInterval));

  // std::string sireneNameForData(name);
  //
  // if (name == "S2")
  //   sireneNameForData = "S1"; // s2 has the same data files than s1
  // else if (name == "S6")
  //   sireneNameForData = "S5"; // s6 has the same data files than s5
  //
  // // S7 (Piccolo) utilise les données vectorInterval de S5
  // std::string vectorIntervalSuffix = sireneNameForData;
  // if (name == "S7") {
  //   vectorIntervalSuffix = "S5";
  // }

  // readDataFromBinaryFile(
  //   dataFilePath,
  //   "dataAmp" + sireneNameForData,
  //   "dataFreq" + sireneNameForData,
  //   "datadureTabs" + sireneNameForData,
  //   "dataVectorInterval" + vectorIntervalSuffix
  // );

  readDataFromBinaryFile(
      dataFilePath,
      data->resourceFileNames.amp,
      data->resourceFileNames.freq,
      data->resourceFileNames.dureTabs,
      data->resourceFileNames.vectorInterval
  );

  // std::cout << "tabFreq[46][20][3] : " << std::fixed << std::setprecision(7) << tabFreq[46][20][3] << std::endl;

  // This are now obtained from sirenCategoryData.synthConstants :
  // if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 24;}
  // else if (name=="S2") {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 24;}
  // else if (name=="S3") {noteMidiCentMax=6400; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 12;}
  // else if (name=="S4") {noteMidiCentMax=6500; pourcentClapetOff=15; noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 12;}
  // else if (name=="S5") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  // else if (name=="S6") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  // else if (name=="S7") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=2.; inertiaFactorTweak = 24;}

  noteMidiCentMax = data->synthConstants.noteMidiCentMax;
  pourcentClapetOff = data->synthConstants.pourcentClapetOff;
  noteMin = data->synthConstants.noteMin;
  coeffPicolo = data->synthConstants.coeffPicolo;
  inertiaFactorTweak = data->synthConstants.inertiaFactorTweak;

  //pat
}

Sirene::~Sirene() {}

void Sirene::setSampleRate(double newSampleRate) {
  sampleRate = newSampleRate;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;
  
  // Recalculer les pitchSchift avec le nouveau sample rate
  if (midiCentVoulue > 0) {
    setMidicent(midiCentVoulue);
  }
}

void Sirene::readDataFromBinaryFile(const std::string& dataFilePath,
                                    const std::string& tabAmpFile,
                                    const std::string& tabFreqFile,
                                    const std::string& dureTabFile,
                                    const std::string& vectorIntervalFile) {

  std::ifstream myfile;
  // todo: check that myfile.gcount() is the expected nb of bytes after read()
  // or see : https://stackoverflow.com/a/2409527
  // or :     https://stackoverflow.com/a/71592110

  // Read tabAmpFile
  myfile.open(dataFilePath + tabAmpFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(tabAmp), sizeof tabAmp);
    if (myfile.gcount() != sizeof tabAmp) {
      // std::cout << "Warning (" << tabAmpFile << ") : readDataFromBinaryFile: read " << myfile.gcount()
      // << " bytes instead of " << sizeof tabAmp << std::endl;
    }
    myfile.close();
  }
  else std::cout <<  "Error. Binary file not found: " <<  dataFilePath + tabAmpFile << "\n";

  // Read dataFreqFile
  myfile.open(dataFilePath + tabFreqFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(tabFreq), sizeof tabFreq);
    myfile.close();
  }
  else std::cout <<  "Error. Binary file not found.\n";

  // Read dureTabFile
  myfile.open(dataFilePath + dureTabFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(dureTabs), sizeof dureTabs);
    myfile.close();
  }
  else std::cout <<  "Error. Binary file not found.\n";

  // Read vectorIntervalFile
  myfile.open(dataFilePath + vectorIntervalFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(vectorInterval), sizeof vectorInterval);
    myfile.close();
  }
  else std::cout <<  "Error. Binary file not found.\n";
}

float Sirene::getCurrentPitch() {
    return noteEncour;
}

void Sirene::setMidicent(int note) {
  midiCentVoulue = note;
  if (midiCentVoulue >= noteMidiCentMax) midiCentVoulue = noteMidiCentMax;
  else if (midiCentVoulue % 100 == 99) midiCentVoulue++;

  // Gauthier added this to enable reading all frames :
  bool gauthierFix = false;
  int newNoteInf = midiCentVoulue / 100;
  if (newNoteInf != noteInf || !gauthierFix) {
      noteInf = newNoteInf;
      noteSup = noteInf + 1;

      // Pat added :
      // Réinitialiser les compteurs de fenêtres FFT pour les nouvelles notes
      countP[noteInf] = 0;
      countP[noteSup] = 0;
      countKInf = 0;
      countKSup = 0;
  }

  pitchSchift[noteInf] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.))  /  (440.0 * pow(2., ((noteInf) - 69.) / 12.)))  * deuxPieSampleRate;
  pitchSchift[noteSup] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.))  /   (440.0 * pow(2., ((noteSup) - 69.) / 12.)))  * deuxPieSampleRate;
}

void Sirene::setnoteFromExt(int note) {
  noteVoulueAvantSlide = note;
  if (noteVoulueAvantSlide > noteMidiCentMax)
    noteVoulueAvantSlide = noteMidiCentMax;
  
  interDepart = static_cast<int>(noteVoulueAvantSlide) - noteEncour;
  
  if (noteEncour > noteVoulueAvantSlide)
    noteEncour = noteEncour - 1;
  else if (noteEncour < noteVoulueAvantSlide)
    noteEncour = noteEncour + 1;
}

int Sirene::computeInertiaBias(SireneSpeedSlideState ouJeSuis){
    switch(ouJeSuis){
        case Montant:
        case TonUpBefore:
        case DemiUpBefore:
        case QuartUpBefore:
        case QuartUpAfter:
            return 1;
        case Descandant:
        case QuartDownBefore:
        case QuartDownAfter:
            return -1;
        case Boucle:
        case jesuisrest:
            return 0;
    }
}

void Sirene::setnote() {
    SireneSpeedSlideState ouJeSuis = oujesuis();
    auto appliedFactor = coeffPicolo;

    bool approx = false;

    ///////////////////////////////////////////////////////////////////////////
    // METHODE GAUTHIER, APROXIMATION /////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    if (approx) {
        auto inertiaBias = computeInertiaBias(ouJeSuis);
        auto inertiaFactor = computeInertiaFactor(noteEncour);

        auto inertiaSpeedToTweak = this->inertiaFactorTweak;

        if(inertiaBias != 0){
            auto vectorIntervalValueNew = inertiaBias * appliedFactor * inertiaFactor * inertiaSpeedToTweak;
            noteEncour=noteEncour+vectorIntervalValueNew;
            switch(ouJeSuis) {
                case Montant:
                case QuartUpBefore:
                case QuartUpAfter: {
                    if (noteEncour > noteVoulueAvantSlide) {
                        noteEncour = noteVoulueAvantSlide;
                    }
                    break;
                }
                case Descandant:
                case QuartDownAfter:
                case QuartDownBefore: {
                    if (noteEncour < noteVoulueAvantSlide) {
                        noteEncour = noteVoulueAvantSlide;
                    }
                    break;
                }
                case TonUpBefore:
                case DemiUpBefore:
                case Boucle:
                case jesuisrest:
                    break;
            }
        }

        setMidicent(noteEncour);
        return;
    }

    ///////////////////////////////////////////////////////////////////////////
    // METHODE PAT, RECONSTITUTION ORIGINALE A PARTIR DE FICHIER DATA /////////
    ///////////////////////////////////////////////////////////////////////////

    // Convertir noteEncour en note entière pour l'indexation (comme dans l'original)
    int note = (int)((noteEncour - 50) / 100.0f);
    if (note < noteMin) note = noteMin;

    // Calculer baseNoteIndex (note - noteMin, comme dans l'original)
    int baseNoteIndex = note - noteMin;

    // Appliquer les formules vectorInterval originales
    if (ouJeSuis == Montant) {
        noteEncour = noteEncour + (100.0f / (vectorInterval[baseNoteIndex + 294] * appliedFactor));
        if(noteEncour > noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
    }
    else if (ouJeSuis == Descandant) {
        noteEncour = noteEncour - (100.0f / (vectorInterval[391 - baseNoteIndex] * appliedFactor));
        if(noteEncour < noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
    }
    else if (ouJeSuis == TonUpBefore) {
        noteEncour = noteEncour + (100.0f / (vectorInterval[((baseNoteIndex + 2) * 6) + 1] * appliedFactor));
    }
    else if (ouJeSuis == DemiUpBefore) {
        noteEncour = noteEncour + (100.0f / (vectorInterval[((baseNoteIndex + 1) * 6) + 2] * appliedFactor));
    }
    else if (ouJeSuis == QuartUpBefore) {
        noteEncour = noteEncour + (100.0f / (vectorInterval[(baseNoteIndex * 6) + 3] * appliedFactor));
        if(noteEncour > noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
    }
    else if (ouJeSuis == Boucle) {
        // Pas de changement
    }
    else if (ouJeSuis == QuartDownAfter) {
        noteEncour = noteEncour - (100.0f / (vectorInterval[(baseNoteIndex * 6) + 4] * appliedFactor));
        if(noteEncour < noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
    }
    else if (ouJeSuis == QuartDownBefore) {
        noteEncour = noteEncour - (100.0f / (vectorInterval[baseNoteIndex * 6] * appliedFactor));
        if(noteEncour < noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
    }
    else if (ouJeSuis == QuartUpAfter) {
        noteEncour = noteEncour + (100.0f / (vectorInterval[(baseNoteIndex * 6) + 5] * appliedFactor));
        if(noteEncour > noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
    }

  setMidicent(noteEncour);
}

// que représentent ces valeurs de 50, 150 et 250 ? -> des MIDI cents
SireneSpeedSlideState Sirene::oujesuis() {
  int inter = static_cast<int>(noteVoulueAvantSlide) - noteEncour;
  SireneSpeedSlideState ouJeSuis = Boucle;
  if (inter == 0)
    ouJeSuis = Boucle;
  else if ((inter - interDepart) > 0 && (inter - interDepart) < 50)
    ouJeSuis = QuartDownAfter;
  else if ((inter - interDepart) < 0 && (inter - interDepart) > -50)
    ouJeSuis = QuartUpAfter;
  else if (inter >= -50 && inter < 0)
    ouJeSuis = QuartDownBefore;
  else if (inter > 250 && (inter - interDepart) <= -50)
    ouJeSuis = Montant;
  else if (inter < -50  && (inter - interDepart) >= 50)
    ouJeSuis = Descandant;
  else if (inter >= 150 && inter < 250)
    ouJeSuis = TonUpBefore;
  else if (inter >= 50 && inter < 150)
    ouJeSuis = DemiUpBefore;
  else if (inter > 0 && inter < 50)
    ouJeSuis = QuartUpBefore;
  return ouJeSuis;
}

void Sirene::changeQualite(int qualt) {
  qualite = qualt;
}

void Sirene::set16ou8Bit(bool is) {
  is16Bit = !is;
}

void Sirene::setVelocite(int velo) {
  ampMax = velo / 500.;
  ampvoulu = (velo / 500.) / (100. / (100 - pourcentClapetOff)) + (pourcentClapetOff / 100.);
}

void Sirene::setisCrossFade(int is) {
  if (is == 0) isCrossfade = false;
  else isCrossfade = true;
}
