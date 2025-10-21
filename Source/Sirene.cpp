
#include "Sirene.h"
#include <JuceHeader.h>

#include <iostream>

// see precision
#include <iomanip>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>


Sirene::Sirene(const std::string& str, const std::string& dataFilePath) :
name(str) {
  // Initialiser le sample rate par défaut à 44.1kHz
  sampleRate = 44100.0;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;
  
  memset(&tabAmp, 0, sizeof(tabAmp));
  memset(&tabFreq, 0, sizeof(tabFreq));
  memset(&dureTabs, 0, sizeof(dureTabs));
  memset(&vectorInterval, 0, sizeof(vectorInterval));

  std::string sireneNameForData(name);

  if (name == "S2")
    sireneNameForData = "S1"; // s2 has the same data files than s1
  else if (name == "S6")
    sireneNameForData = "S5"; // s6 has the same data files than s5
	
  // both methods should end up loading data in the float arrays :

  // veeeeeery slow to load, failed attempt :
  // readDataFromBinaryData(sireneNameForData);

  // we read from files instead, still slow but acceptable :
  // S7 (Piccolo) utilise les données vectorInterval de S5
  std::string vectorIntervalSuffix = sireneNameForData;
  if (name == "S7") {
    vectorIntervalSuffix = "S5";
  }
  
	readDataFromBinaryFile(
    dataFilePath,
    "dataAmp" + sireneNameForData,
    "dataFreq" + sireneNameForData,
    "datadureTabs" + sireneNameForData,
    "dataVectorInterval" + vectorIntervalSuffix
  );

       // Sirene constructor

  if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 32;}
  else if (name=="S2") {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 32;}
  else if (name=="S3") {noteMidiCentMax=6400; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 28;}
  else if (name=="S4") {noteMidiCentMax=6500; pourcentClapetOff=15; noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 28;}
  else if (name=="S5") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  else if (name=="S6") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  else if (name=="S7") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=2.; inertiaFactorTweak = 36;}

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

void Sirene::readDataFromBinaryFile(std::string dataFilePath, std::string tabAmpFile, std::string tabFreqFile, std::string dureTabFile, std::string vectorIntervalFile){

  std::ifstream myfile;
  bool allLoaded = true;

  // Read tabAmpFile
  std::string fullPath = dataFilePath + tabAmpFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(tabAmp), sizeof tabAmp);
    myfile.close();
  }
  else { 
    DBG("✗ FAILED to load " << fullPath);
    allLoaded = false;
  }

  // Read dataFreqFile
  fullPath = dataFilePath + tabFreqFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(tabFreq), sizeof tabFreq);
    myfile.close();
  }
  else { 
    DBG("✗ FAILED to load " << fullPath);
    allLoaded = false;
  }

  // Read dureTabFile
  fullPath = dataFilePath + dureTabFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(dureTabs), sizeof dureTabs);
    myfile.close();
  }
  else { 
    DBG("✗ FAILED to load " << fullPath);
    allLoaded = false;
  }

  // Read vectorIntervalFile
  fullPath = dataFilePath + vectorIntervalFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(vectorInterval), sizeof vectorInterval);
    myfile.close();
  }
  else { 
    DBG("✗ FAILED to load " << fullPath);
    allLoaded = false;
  }
  
  // Log seulement en cas d'erreur
  if (!allLoaded) {
    DBG("✗ Some resources failed to load for " << name);
  }

}

void Sirene::setMidicent(int note) {
  midiCentVoulue = note;
  if (midiCentVoulue >= noteMidiCentMax) midiCentVoulue = noteMidiCentMax;
  else if (midiCentVoulue % 100 == 99) midiCentVoulue++;
  noteInf = midiCentVoulue / 100;
  noteSup = noteInf + 1;
  
  // Réinitialiser les compteurs de fenêtres FFT pour les nouvelles notes
  countP[noteInf] = 0;
  countP[noteSup] = 0;
  countKInf = 0;
  countKSup = 0;
  
  pitchSchift[noteInf] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.)) / (440.0 * pow(2., ((noteInf) - 69.) / 12.))) * deuxPieSampleRate;
  pitchSchift[noteSup] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.)) / (440.0 * pow(2., ((noteSup) - 69.) / 12.))) * deuxPieSampleRate;
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
  
  // Convertir noteEncour en note entière pour l'indexation (comme dans l'original)
  int note = (int)((noteEncour-50)/100.);
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
  // Set velocity
  ampMax = velo / 500.;
  ampvoulu = (velo / 500.) / (100. / (100 - pourcentClapetOff)) + (pourcentClapetOff / 100.);
}

void Sirene::setisCrossFade(int is) {
  if (is == 0) isCrossfade = false;
  else isCrossfade = true;
}


