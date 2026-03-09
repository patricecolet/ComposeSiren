#include "Sirene.h"

#include <cstring>
#include <fstream>
#include <stdexcept>

Sirene::Sirene(const std::string& str, const std::string& dataFilePath)
    : name(str) {
  sampleRate = 44100.0;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;

  std::memset(&tabAmp, 0, sizeof(tabAmp));
  std::memset(&tabFreq, 0, sizeof(tabFreq));
  std::memset(&dureTabs, 0, sizeof(dureTabs));
  std::memset(&vectorInterval, 0, sizeof(vectorInterval));

  std::string sireneNameForData(name);
  if (name == "S2") {
    sireneNameForData = "S1";
  } else if (name == "S6") {
    sireneNameForData = "S5";
  }

  std::string vectorIntervalSuffix = sireneNameForData;
  if (name == "S7") {
    vectorIntervalSuffix = "S5";
  }

  readDataFromBinaryFile(
      dataFilePath,
      "dataAmp" + sireneNameForData,
      "dataFreq" + sireneNameForData,
      "datadureTabs" + sireneNameForData,
      "dataVectorInterval" + vectorIntervalSuffix);

  if (name == "S1") {
    noteMidiCentMax = 7200;
    pourcentClapetOff = 7;
    noteMin = 24;
    coeffPicolo = 1.0f;
    inertiaFactorTweak = 32;
  } else if (name == "S2") {
    noteMidiCentMax = 7200;
    pourcentClapetOff = 7;
    noteMin = 24;
    coeffPicolo = 1.0f;
    inertiaFactorTweak = 32;
  } else if (name == "S3") {
    noteMidiCentMax = 6400;
    pourcentClapetOff = 7;
    noteMin = 24;
    coeffPicolo = 1.0f;
    inertiaFactorTweak = 28;
  } else if (name == "S4") {
    noteMidiCentMax = 6500;
    pourcentClapetOff = 15;
    noteMin = 24;
    coeffPicolo = 1.0f;
    inertiaFactorTweak = 28;
  } else if (name == "S5") {
    noteMidiCentMax = 7900;
    pourcentClapetOff = 7;
    noteMin = 36;
    coeffPicolo = 1.0f;
    inertiaFactorTweak = 48;
  } else if (name == "S6") {
    noteMidiCentMax = 7900;
    pourcentClapetOff = 7;
    noteMin = 36;
    coeffPicolo = 1.0f;
    inertiaFactorTweak = 48;
  } else if (name == "S7") {
    noteMidiCentMax = 7900;
    pourcentClapetOff = 7;
    noteMin = 36;
    coeffPicolo = 2.0f;
    inertiaFactorTweak = 36;
  }
}

Sirene::~Sirene() {}

void Sirene::setSampleRate(double newSampleRate) {
  sampleRate = newSampleRate;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;
  if (midiCentVoulue > 0) {
    setMidicent(midiCentVoulue);
  }
}

void Sirene::readDataFromBinaryFile(
    std::string dataFilePath,
    std::string tabAmpFile,
    std::string tabFreqFile,
    std::string dureTabFile,
    std::string vectorIntervalFile) {
  std::ifstream myfile;
  bool allLoaded = true;

  std::string fullPath = dataFilePath + tabAmpFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open()) {
    myfile.read(reinterpret_cast<char*>(tabAmp), sizeof tabAmp);
    myfile.close();
  } else {
    allLoaded = false;
  }

  fullPath = dataFilePath + tabFreqFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open()) {
    myfile.read(reinterpret_cast<char*>(tabFreq), sizeof tabFreq);
    myfile.close();
  } else {
    allLoaded = false;
  }

  fullPath = dataFilePath + dureTabFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open()) {
    myfile.read(reinterpret_cast<char*>(dureTabs), sizeof dureTabs);
    myfile.close();
  } else {
    allLoaded = false;
  }

  fullPath = dataFilePath + vectorIntervalFile;
  myfile.open(fullPath, std::ios::binary);
  if (myfile.is_open()) {
    myfile.read(reinterpret_cast<char*>(vectorInterval), sizeof vectorInterval);
    myfile.close();
  } else {
    allLoaded = false;
  }

  if (!allLoaded) {
    throw std::runtime_error("Failed to load siren resources for " + name + " at " + dataFilePath);
  }
}

void Sirene::setMidicent(int note) {
  midiCentVoulue = note;
  if (midiCentVoulue >= noteMidiCentMax) {
    midiCentVoulue = noteMidiCentMax;
  } else if (midiCentVoulue % 100 == 99) {
    midiCentVoulue++;
  }
  noteInf = midiCentVoulue / 100;
  noteSup = noteInf + 1;

  countP[noteInf] = 0;
  countP[noteSup] = 0;
  countKInf = 0;
  countKSup = 0;

  pitchSchift[noteInf] =
      ((440.0 * std::pow(2.0, ((midiCentVoulue / 100.0) - 69.0) / 12.0)) /
       (440.0 * std::pow(2.0, ((noteInf)-69.0) / 12.0))) *
      deuxPieSampleRate;
  pitchSchift[noteSup] =
      ((440.0 * std::pow(2.0, ((midiCentVoulue / 100.0) - 69.0) / 12.0)) /
       (440.0 * std::pow(2.0, ((noteSup)-69.0) / 12.0))) *
      deuxPieSampleRate;
}

void Sirene::setnoteFromExt(int note) {
  noteVoulueAvantSlide = note;
  if (noteVoulueAvantSlide > noteMidiCentMax) {
    noteVoulueAvantSlide = noteMidiCentMax;
  }

  interDepart = static_cast<int>(noteVoulueAvantSlide) - noteEncour;

  if (noteEncour > noteVoulueAvantSlide) {
    noteEncour = noteEncour - 1;
  } else if (noteEncour < noteVoulueAvantSlide) {
    noteEncour = noteEncour + 1;
  }
}

int Sirene::computeInertiaBias(SireneSpeedSlideState ouJeSuis) {
  switch (ouJeSuis) {
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
  return 0;
}

void Sirene::setnote() {
  SireneSpeedSlideState ouJeSuis = oujesuis();
  auto appliedFactor = coeffPicolo;

  int note = static_cast<int>((noteEncour - 50) / 100.0);
  if (note < noteMin) {
    note = noteMin;
  }
  int baseNoteIndex = note - noteMin;

  if (ouJeSuis == Montant) {
    noteEncour = noteEncour + (100.0f / (vectorInterval[baseNoteIndex + 294] * appliedFactor));
    if (noteEncour > noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  } else if (ouJeSuis == Descandant) {
    noteEncour = noteEncour - (100.0f / (vectorInterval[391 - baseNoteIndex] * appliedFactor));
    if (noteEncour < noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  } else if (ouJeSuis == TonUpBefore) {
    noteEncour = noteEncour + (100.0f / (vectorInterval[((baseNoteIndex + 2) * 6) + 1] * appliedFactor));
  } else if (ouJeSuis == DemiUpBefore) {
    noteEncour = noteEncour + (100.0f / (vectorInterval[((baseNoteIndex + 1) * 6) + 2] * appliedFactor));
  } else if (ouJeSuis == QuartUpBefore) {
    noteEncour = noteEncour + (100.0f / (vectorInterval[(baseNoteIndex * 6) + 3] * appliedFactor));
    if (noteEncour > noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  } else if (ouJeSuis == QuartDownAfter) {
    noteEncour = noteEncour - (100.0f / (vectorInterval[(baseNoteIndex * 6) + 4] * appliedFactor));
    if (noteEncour < noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  } else if (ouJeSuis == QuartDownBefore) {
    noteEncour = noteEncour - (100.0f / (vectorInterval[baseNoteIndex * 6] * appliedFactor));
    if (noteEncour < noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  } else if (ouJeSuis == QuartUpAfter) {
    noteEncour = noteEncour + (100.0f / (vectorInterval[(baseNoteIndex * 6) + 5] * appliedFactor));
    if (noteEncour > noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  }

  setMidicent(static_cast<int>(noteEncour));
}

SireneSpeedSlideState Sirene::oujesuis() {
  int inter = static_cast<int>(noteVoulueAvantSlide) - noteEncour;
  SireneSpeedSlideState ouJeSuis = Boucle;
  if (inter == 0) {
    ouJeSuis = Boucle;
  } else if ((inter - interDepart) > 0 && (inter - interDepart) < 50) {
    ouJeSuis = QuartDownAfter;
  } else if ((inter - interDepart) < 0 && (inter - interDepart) > -50) {
    ouJeSuis = QuartUpAfter;
  } else if (inter >= -50 && inter < 0) {
    ouJeSuis = QuartDownBefore;
  } else if (inter > 250 && (inter - interDepart) <= -50) {
    ouJeSuis = Montant;
  } else if (inter < -50 && (inter - interDepart) >= 50) {
    ouJeSuis = Descandant;
  } else if (inter >= 150 && inter < 250) {
    ouJeSuis = TonUpBefore;
  } else if (inter >= 50 && inter < 150) {
    ouJeSuis = DemiUpBefore;
  } else if (inter > 0 && inter < 50) {
    ouJeSuis = QuartUpBefore;
  }
  return ouJeSuis;
}

void Sirene::changeQualite(int qualt) { qualite = qualt; }

void Sirene::set16ou8Bit(bool is) { is16Bit = !is; }

void Sirene::setVelocite(int velo) {
  ampMax = velo / 500.0f;
  ampvoulu = (velo / 500.0f) / (100.0f / (100 - pourcentClapetOff)) + (pourcentClapetOff / 100.0f);
}

void Sirene::setisCrossFade(int is) { isCrossfade = (is != 0); }
