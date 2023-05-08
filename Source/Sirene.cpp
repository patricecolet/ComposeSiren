/*
  ==============================================================================

    Sirene.cpp
    Created: 6 May 2020 10:49:23am
    Author:  guyot

  ==============================================================================
*/

#include "Sirene.h"
#include <JuceHeader.h>
// #include <BinaryData.h>

#include <iostream>

// see precision
#include <iomanip>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>

/*
#define MAX_Partiel 200
#define NOMBRE_DE_NOTE 80
#define MAX_TAB 1000
#define pourcentClapetOff 7

float tabAmp[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
float tabFreq[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
float dureTabs[NOMBRE_DE_NOTE][3];//0=dureTab en samples // 1=nombreMax de Tab // 2=FreqMoyenne
float vector_interval[392];
*/


//Timer myTimer;

Sirene::Sirene(const std::string& str, const std::string& dataFilePath) :
name(str) {
  memset(&tabAmp, 0, sizeof(tabAmp));
  memset(&tabFreq, 0, sizeof(tabFreq));
  memset(&dureTabs, 0, sizeof(dureTabs));

  std::string sireneNameForData(name);

  if (name == "S2")
    sireneNameForData = "S1"; // s2 has the same data files than s1
  else if (name == "S6")
    sireneNameForData = "S5"; // s6 has the same data files than s5
	
  // both methods should end up loading data in the float arrays :

  // veeeeeery slow to load, failed attempt :
  // readDataFromBinaryData(sireneNameForData);

  // we read from files instead, still slow but acceptable :
	readDataFromBinaryFile(
    dataFilePath,
    "dataAmp" + sireneNameForData,
    "dataFreq" + sireneNameForData,
    "datadureTabs" + sireneNameForData
  );

  std::cout << "tabFreq[46][20][3] : " << std::fixed << std::setprecision(7) << tabFreq[46][20][3] << std::endl;

  if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 24;}
  else if (name=="S2") {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 24;}
  else if (name=="S3") {noteMidiCentMax=6400; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 12;}
  else if (name=="S4") {noteMidiCentMax=6500; pourcentClapetOff=15; noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 12;}
  else if (name=="S5") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  else if (name=="S6") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  else if (name=="S7") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=2.; inertiaFactorTweak = 24;}

  //pat
  /*
  interDepart = 0;
  boutonveloONS1 = false;
  isCrossfade = false;
  noteVoulueAvantSlide = 0;
  noteEncour = 0;
  ouJeSuis = 0;
  //*/
}

Sirene::~Sirene() {}

/*
void Sirene::readDataFromBinaryData(std::string id) {
  // will be passed by reference to and set by BinaryData::getNamedResource()
  int ampSize, freqSize, dureTabsSize, vectorIntervalSize;
  MemoryBlock block;

  const char* ampData =
    BinaryData::getNamedResource(("dataAmp" + id).c_str(), ampSize);
  block = MemoryBlock(static_cast<const void*>(ampData), ampSize);
  block.copyTo(static_cast<void*>(tabAmp), 0, ampSize);

  const char* freqData =
    BinaryData::getNamedResource(("dataFreq" + id).c_str(), freqSize);
  block = MemoryBlock(static_cast<const void*>(freqData), freqSize);
  block.copyTo(static_cast<void*>(tabFreq), 0, freqSize);

  const char* dureTabsData =
    BinaryData::getNamedResource(("datadureTabs" + id).c_str(), dureTabsSize);
  block = MemoryBlock(static_cast<const void*>(dureTabsData), dureTabsSize);
  block.copyTo(static_cast<void*>(dureTabs), 0, dureTabsSize);

  const char* vectorIntervalData =
    BinaryData::getNamedResource(("dataVectorInterval" + id).c_str(), vectorIntervalSize);
  block = MemoryBlock(static_cast<const void*>(vectorIntervalData), vectorIntervalSize);
  block.copyTo(static_cast<void*>(vector_interval), 0, vectorIntervalSize);
}
//*/

void Sirene::readDataFromBinaryFile(std::string dataFilePath, std::string tabAmpFile, std::string tabFreqFile, std::string dureTabFile){

  std::ifstream myfile;

  // Read tabAmpFile
  myfile.open(dataFilePath + tabAmpFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(tabAmp), sizeof tabAmp); // todo: check that input.gcount() is the number of bytes expected
    myfile.close();
  }
  else std::cout <<  "Error. Binary file not found: " <<  dataFilePath + tabAmpFile << "\n";

  // Read dataFreqFile
  myfile.open(dataFilePath + tabFreqFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(tabFreq), sizeof tabFreq); // todo: check that input.gcount() is the number of bytes expected
    myfile.close();
  }
  else std::cout <<  "Error. Binary file not found.\n";

  // Read dureTabFile
  myfile.open(dataFilePath + dureTabFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(dureTabs), sizeof dureTabs); // todo: check that input.gcount() is the number of bytes expected
    myfile.close();
  }
  else std::cout <<  "Error. Binary file not found.\n";

}

void Sirene::setMidicent(int note) {
  midiCentVoulue = note;
  if (midiCentVoulue >= noteMidiCentMax) midiCentVoulue = noteMidiCentMax;
  else if (midiCentVoulue % 100 == 99) midiCentVoulue++;
  noteInf = midiCentVoulue / 100;
  noteSup = noteInf + 1;
  pitchSchift[noteInf] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.))  /  (440.0 * pow(2., ((noteInf) - 69.) / 12.)))  * DeuxPieSampleRate;
  pitchSchift[noteSup] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.))  /   (440.0 * pow(2., ((noteSup) - 69.) / 12.)))  * DeuxPieSampleRate;
  // std::cout << "noteInf:" << noteInf << std::endl;
}

void Sirene::setnoteFromExt(int note) {
  //std::cout << "Note from ext: " << note << std::endl;
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
  auto inertiaBias = computeInertiaBias(ouJeSuis);
  auto inertiaFactor = computeInertiaFactor(noteEncour);

  auto inertiaSpeedToTweak = this->inertiaFactorTweak;
  if(inertiaBias != 0){
      auto vectorIntervalValueNew = inertiaBias * appliedFactor * inertiaFactor * inertiaSpeedToTweak;
      noteEncour=noteEncour+vectorIntervalValueNew;
      switch(ouJeSuis){
        case Montant:
        case QuartUpBefore:
        case QuartUpAfter:
            if(noteEncour > noteVoulueAvantSlide)noteEncour=noteVoulueAvantSlide;
            break;
        case Descandant:
        case QuartDownAfter:
        case QuartDownBefore:
            if(noteEncour < noteVoulueAvantSlide)noteEncour=noteVoulueAvantSlide;
              break;
        case TonUpBefore:
        case DemiUpBefore:
        case Boucle:
        case jesuisrest:
              break;
      }
    }

  setMidicent(noteEncour);
}

SireneSpeedSlideState Sirene::oujesuis() {
  int inter = static_cast<int>(noteVoulueAvantSlide) - noteEncour;
  SireneSpeedSlideState ouJeSuis;
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
  // printf("velo:%i\n",velo);
  ampMax = velo / 500.;
  ampvoulu = (velo / 500.) / (100. / (100 - pourcentClapetOff)) + (pourcentClapetOff / 100.);
}

void Sirene::setisCrossFade(int is) {
  if (is == 0) isCrossfade = false;
  else isCrossfade = true;
}
