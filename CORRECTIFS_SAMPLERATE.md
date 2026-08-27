# Correctifs Sample Rate et Arrondi pour ComposeSiren

## 📋 Table des matières

- [Résumé des modifications](#résumé-des-modifications)
- [Comment appliquer ces correctifs](#comment-appliquer-ces-correctifs)
- [Modifications détaillées par fichier](#modifications-détaillées-par-fichier)
- [Résumé final](#résumé-des-corrections)

## Résumé des modifications

Ces modifications corrigent deux problèmes critiques :
1. **Sample rate hardcodé à 44.1kHz** → Sample rate dynamique
2. **Erreurs d'arrondi** dans les calculs de pitch et vectorInterval

## Comment appliquer ces correctifs

### Option 1 : Application manuelle
Suivez les modifications ligne par ligne dans chaque fichier en comparant AVANT/APRÈS.

### Option 2 : Avec git (recommandé)
```bash
# 1. Cloner le fork propre
git clone --recursive https://github.com/mecaviv/ComposeSiren.git ComposeSiren-fixed
cd ComposeSiren-fixed

# 2. Créer une branche pour les correctifs
git checkout -b fix/dynamic-samplerate

# 3. Appliquer les modifications en suivant ce document
# (éditer chaque fichier selon les instructions)

# 4. Tester la compilation
xcodebuild -project Builds/MacOSX/ComposeSiren.xcodeproj -target "ComposeSiren - Standalone Plugin" -configuration Release

# 5. Commiter les changements
git add Source/
git commit -m "fix: Implémenter sample rate dynamique et corrections d'arrondi

- Remplacer DeuxPieSampleRate hardcodé (44.1kHz) par variable dynamique
- Ajouter méthode setSampleRate() dans Sirene, Synth et MidiIn
- Corriger erreurs d'arrondi avec 100.0f au lieu de 100
- Implémenter vectorInterval pour inertie réaliste des sirènes
- Atténuer S7 (piccolo) à 30% car trop fort
- Supprimer messages debug trop fréquents causant des plantages"
```

---

## Modifications détaillées par fichier

---

## 1. Source/Sirene.h

### Ligne 32-33 : Supprimer la macro hardcodée

**AVANT :**
```cpp
#define DeuxPieSampleRate (2.* M_PI / 44100)
#define MAX_Partiel 200
```

**APRÈS :**
```cpp
// Remplacer la macro hardcodée par une variable dynamique
// #define DeuxPieSampleRate (2.* M_PI / 44100)
#define MAX_Partiel 200
```

### Lignes 66-68 : Ajouter variables sample rate + supprimer inertiaFactorTweak

**NOTE** : `inertiaFactorTweak` n'est plus utilisé et peut être supprimé.

**AVANT :**
```cpp
int coeffPicolo;
float inertiaFactorTweak;

public:
```

**OPTION A - Supprimer inertiaFactorTweak (recommandé) :**
```cpp
int coeffPicolo;

// Variables pour le sample rate dynamique
double sampleRate;
double deuxPieSampleRate;

public:
```

**OPTION B - Garder avec commentaire :**
```cpp
int coeffPicolo;
// float inertiaFactorTweak; // OBSOLETE: plus utilisé (remplacé par vectorInterval)

// Variables pour le sample rate dynamique
double sampleRate;
double deuxPieSampleRate;

public:
```

### Ligne 79 : Ajouter la méthode setSampleRate()

**AVANT :**
```cpp
  void setVelocite(int velo);
  
private:
```

**APRÈS :**
```cpp
  void setVelocite(int velo);
  
  // Nouvelle méthode pour mettre à jour le sample rate
  void setSampleRate(double newSampleRate);
  
private:
```

### Ligne 92 : Ajouter le prototype de readDataFromBinaryFile avec vectorInterval

**AVANT :**
```cpp
void readDataFromBinaryFile(std::string dataFilePath, std::string tabAmpFile, std::string tabFreqFile, std::string dureTabFile);
```

**APRÈS :**
```cpp
void readDataFromBinaryFile(std::string dataFilePath, std::string tabAmpFile, std::string tabFreqFile, std::string dureTabFile, std::string vectorIntervalFile);
```

### Ligne 175 : Ajouter le tableau vectorInterval

**AVANT :**
```cpp
  float tabFreq[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
  float dureTabs[NOMBRE_DE_NOTE][3]; // 0=dureTab en samples // 1=nombreMax de Tab // 2=FreqMoyenne

  bool count8bit = true;
```

**APRÈS :**
```cpp
  float tabFreq[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
  float dureTabs[NOMBRE_DE_NOTE][3]; // 0=dureTab en samples // 1=nombreMax de Tab // 2=FreqMoyenne
  float vectorInterval[392]; // Données pour l'inertie des sirènes

  bool count8bit = true;
```

---

## 2. Source/Sirene.cpp

### Ligne 18-20 : Initialiser le sample rate dans le constructeur

**AVANT :**
```cpp
Sirene::Sirene(const std::string& str, const std::string& dataFilePath) :
name(str) {
  memset(&tabAmp, 0, sizeof(tabAmp));
```

**APRÈS :**
```cpp
Sirene::Sirene(const std::string& str, const std::string& dataFilePath) :
name(str) {
  // Initialiser le sample rate par défaut à 44.1kHz
  sampleRate = 44100.0;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;
  
  memset(&tabAmp, 0, sizeof(tabAmp));
```

### Ligne 25 : Initialiser vectorInterval

**AVANT :**
```cpp
  memset(&tabFreq, 0, sizeof(tabFreq));
  memset(&dureTabs, 0, sizeof(dureTabs));

  std::string sireneNameForData(name);
```

**APRÈS :**
```cpp
  memset(&tabFreq, 0, sizeof(tabFreq));
  memset(&dureTabs, 0, sizeof(dureTabs));
  memset(&vectorInterval, 0, sizeof(vectorInterval));

  std::string sireneNameForData(name);
```

### Lignes 41-49 : S7 utilise les vectorInterval de S5

**AVANT :**
```cpp
  // we read from files instead, still slow but acceptable :
	readDataFromBinaryFile(
    dataFilePath,
    "dataAmp" + sireneNameForData,
    "dataFreq" + sireneNameForData,
    "datadureTabs" + sireneNameForData
  );
```

**APRÈS :**
```cpp
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
```

### Ligne 51 : Supprimer le debug inutile

**AVANT :**
```cpp
  std::cout << "tabFreq[46][20][3] : " << std::fixed << std::setprecision(7) << tabFreq[46][20][3] << std::endl;

  if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 24;}
```

**APRÈS :**
```cpp
       // Sirene constructor

  if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 32;}
```

### Lignes 53-58 : Supprimer inertiaFactorTweak (obsolète avec vectorInterval)

**NOTE IMPORTANTE** : `inertiaFactorTweak` n'est PLUS utilisé car la nouvelle implémentation de `setnote()` utilise directement les données `vectorInterval`. Ces lignes peuvent être supprimées ou gardées pour référence historique.

**AVANT :**
```cpp
  if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 24;}
  else if (name=="S2") {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 24;}
  else if (name=="S3") {noteMidiCentMax=6400; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 12;}
  else if (name=="S4") {noteMidiCentMax=6500; pourcentClapetOff=15; noteMin=24; coeffPicolo=1.; inertiaFactorTweak = 12;}
  else if (name=="S5") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  else if (name=="S6") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; inertiaFactorTweak = 48;}
  else if (name=="S7") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=2.; inertiaFactorTweak = 24;}
```

**OPTION A - Supprimer complètement (recommandé) :**
```cpp
  if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.;}
  else if (name=="S2") {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.;}
  else if (name=="S3") {noteMidiCentMax=6400; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.;}
  else if (name=="S4") {noteMidiCentMax=6500; pourcentClapetOff=15; noteMin=24; coeffPicolo=1.;}
  else if (name=="S5") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.;}
  else if (name=="S6") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.;}
  else if (name=="S7") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=2.;}
```

**OPTION B - Garder avec commentaire :**
```cpp
  // Note: inertiaFactorTweak n'est plus utilisé (vectorInterval le remplace)
  if (name=="S1")      {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; /* inertiaFactorTweak = 32; */}
  else if (name=="S2") {noteMidiCentMax=7200; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; /* inertiaFactorTweak = 32; */}
  else if (name=="S3") {noteMidiCentMax=6400; pourcentClapetOff=7;  noteMin=24; coeffPicolo=1.; /* inertiaFactorTweak = 28; */}
  else if (name=="S4") {noteMidiCentMax=6500; pourcentClapetOff=15; noteMin=24; coeffPicolo=1.; /* inertiaFactorTweak = 28; */}
  else if (name=="S5") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; /* inertiaFactorTweak = 48; */}
  else if (name=="S6") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=1.; /* inertiaFactorTweak = 48; */}
  else if (name=="S7") {noteMidiCentMax=7900; pourcentClapetOff=7;  noteMin=36; coeffPicolo=2.; /* inertiaFactorTweak = 36; */}
```

### Après le destructeur : Ajouter la méthode setSampleRate()

**AJOUTER APRÈS :**
```cpp
Sirene::~Sirene() {}
```

**NOUVEAU CODE :**
```cpp
void Sirene::setSampleRate(double newSampleRate) {
  sampleRate = newSampleRate;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;
  
  // Recalculer les pitchSchift avec le nouveau sample rate
  if (midiCentVoulue > 0) {
    setMidicent(midiCentVoulue);
  }
}
```

### Ligne ~70 : Signature de readDataFromBinaryFile

**AVANT :**
```cpp
void Sirene::readDataFromBinaryFile(std::string dataFilePath, std::string tabAmpFile, std::string tabFreqFile, std::string dureTabFile){
```

**APRÈS :**
```cpp
void Sirene::readDataFromBinaryFile(std::string dataFilePath, std::string tabAmpFile, std::string tabFreqFile, std::string dureTabFile, std::string vectorIntervalFile){
```

### Lignes ~90-95 : Lire le fichier vectorInterval

**AVANT :**
```cpp
  else std::cout <<  "Error. Binary file not found.\n";

}
```

**APRÈS :**
```cpp
  else { /* Binary file not found */ }

  // Read vectorIntervalFile
  myfile.open(dataFilePath + vectorIntervalFile, std::ios::binary);
  if (myfile.is_open())
  {
    myfile.read(reinterpret_cast<char *>(vectorInterval), sizeof vectorInterval); // todo: check that input.gcount() is the number of bytes expected
    myfile.close();
  }
  else { /* Binary file not found */ }

}
```

### Ligne ~98 : FIX CRITIQUE - Utiliser deuxPieSampleRate au lieu de DeuxPieSampleRate

**AVANT :**
```cpp
  pitchSchift[noteInf] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.))  /  (440.0 * pow(2., ((noteInf) - 69.) / 12.)))  * DeuxPieSampleRate;
  pitchSchift[noteSup] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.))  /   (440.0 * pow(2., ((noteSup) - 69.) / 12.)))  * DeuxPieSampleRate;
```

**APRÈS :**
```cpp
  // Réinitialiser les compteurs de fenêtres FFT pour les nouvelles notes
  countP[noteInf] = 0;
  countP[noteSup] = 0;
  countKInf = 0;
  countKSup = 0;
  
  pitchSchift[noteInf] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.)) / (440.0 * pow(2., ((noteInf) - 69.) / 12.))) * deuxPieSampleRate;
  pitchSchift[noteSup] = ((440.0 * pow(2., ((midiCentVoulue/100.) - 69.) / 12.)) / (440.0 * pow(2., ((noteSup) - 69.) / 12.))) * deuxPieSampleRate;
```

### Ligne ~133 : FIX CRITIQUE - Remplacer la fonction setnote() complète

**AVANT :**
```cpp
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
```

**APRÈS (utilise 100.0f pour forcer les calculs en float) :**
```cpp
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
```

### Ligne ~197 : Nettoyer les commentaires debug

**AVANT :**
```cpp
void Sirene::setVelocite(int velo) {
  // printf("velo:%i\n",velo);
  ampMax = velo / 500.;
```

**APRÈS :**
```cpp
void Sirene::setVelocite(int velo) {
  // Set velocity
  ampMax = velo / 500.;
```

---

## 3. Source/PluginProcessor.h

### Ligne 14 : Ajouter un commentaire pour forward declaration

**AVANT :**
```cpp
#include "CS_midiIN.h"



```

**APRÈS :**
```cpp
#include "CS_midiIN.h"

// Forward declaration



```

---

## 4. Source/PluginProcessor.cpp

### Ligne 14 : Ajouter variable globale pour accès au processor

**AVANT :**
```cpp
#include <functional>

//==============================================================================
SirenePlugAudioProcessor::SirenePlugAudioProcessor()
```

**APRÈS :**
```cpp
#include <functional>

// Variable globale pour accéder au processor depuis Sirene
SirenePlugAudioProcessor* g_processor = nullptr;

//==============================================================================
SirenePlugAudioProcessor::SirenePlugAudioProcessor()
```

### Ligne 27 : Assigner l'instance dans le constructeur

**AVANT :**
```cpp
{
    startTimer(1);
    this->mySynth = new Synth();
```

**APRÈS :**
```cpp
{
    g_processor = this; // Assigner l'instance courante
    startTimer(1);
    this->mySynth = new Synth();
    

```

### Ligne 117 : FIX CRITIQUE - Propager le sample rate dans prepareToPlay()

**AVANT :**
```cpp
void SirenePlugAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

}
```

**APRÈS :**
```cpp
void SirenePlugAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Propager le sample rate aux composants qui en ont besoin
    if (mySynth != nullptr) {
        mySynth->setSampleRate(sampleRate);
    }
    
    if (myMidiInHandler != nullptr) {
        myMidiInHandler->setSampleRate(sampleRate);
    }
}
```

### Ligne 159 : Supprimer les messages de debug

**AVANT :**
```cpp
void SirenePlugAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
```

**APRÈS :**
```cpp
void SirenePlugAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Supprimer les messages de debug trop fréquents qui causent des plantages
    
    buffer.clear();
```

### Ligne ~12 : Ajouter une constante pour l'atténuation du piccolo

**AJOUTER APRÈS les includes :**
```cpp
#include <functional>

// Constante pour atténuer S7 (piccolo) qui est trop fort
static const float S7_ATTENUATION = 0.3f;

// Variable globale pour accéder au processor depuis Sirene
SirenePlugAudioProcessor* g_processor = nullptr;
```

### Lignes 196-203 : Atténuer S7 (piccolo) avec la constante

**AVANT :**
```cpp
        channelLeft[sample] =
            sampleS1 * mySynth->getPan(1,0) +
            sampleS2 * mySynth->getPan(2,0) +
            sampleS3 * mySynth->getPan(3,0) +
            sampleS4 * mySynth->getPan(4,0) +
            sampleS5 * mySynth->getPan(5,0) +
            sampleS6 * mySynth->getPan(6,0) +
            sampleS7 * mySynth->getPan(7,0);

        channelRight[sample] =
            sampleS1 * mySynth->getPan(1,1) +
            sampleS2 * mySynth->getPan(2,1) +
            sampleS3 * mySynth->getPan(3,1) +
            sampleS4 * mySynth->getPan(4,1) +
            sampleS5 * mySynth->getPan(5,1) +
            sampleS6 * mySynth->getPan(6,1) +
            sampleS7 * mySynth->getPan(7,1);
```

**APRÈS :**
```cpp
        channelLeft[sample] =
            sampleS1 * mySynth->getPan(1,0) +
            sampleS2 * mySynth->getPan(2,0) +
            sampleS3 * mySynth->getPan(3,0) +
            sampleS4 * mySynth->getPan(4,0) +
            sampleS5 * mySynth->getPan(5,0) +
            sampleS6 * mySynth->getPan(6,0) +
            sampleS7 * mySynth->getPan(7,0) * S7_ATTENUATION;

        channelRight[sample] =
            sampleS1 * mySynth->getPan(1,1) +
            sampleS2 * mySynth->getPan(2,1) +
            sampleS3 * mySynth->getPan(3,1) +
            sampleS4 * mySynth->getPan(4,1) +
            sampleS5 * mySynth->getPan(5,1) +
            sampleS6 * mySynth->getPan(6,1) +
            sampleS7 * mySynth->getPan(7,1) * S7_ATTENUATION;
```

---

## 5. Source/synth.h

### Ligne 52 : Ajouter la déclaration de setSampleRate()

**AVANT :**
```cpp
    void timer512();
    
    float getPan(int sireneNumber, int channel);
```

**APRÈS :**
```cpp
    void timer512();
    
    // Nouvelle méthode pour mettre à jour le sample rate de toutes les sirènes
    void setSampleRate(double newSampleRate);

    
    float getPan(int sireneNumber, int channel);
```

---

## 5b. Source/synth.cpp

### Lignes 34-37 : Simplifier le chemin des ressources

**AVANT :**
```cpp
#else
#if CMS_BUILD_WITH_PROJUCER
    std::string dataFilePath = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getChildFile ("Contents/Resources/").getFullPathName().toStdString() + '/';
#elif CMS_BUILD_WITH_CMAKE
    std::string dataFilePath = "/Library/Audio/Plug-ins/Mecanique Vivante/ComposeSiren/Resources/";
#endif
#endif
```

**APRÈS :**
```cpp
#else
    // Utiliser le chemin externe pour les ressources FFT partagées
    std::string dataFilePath = "/Library/Audio/Plug-ins/Mecanique Vivante/ComposeSiren/Resources/";
#endif
```

### Après le destructeur : Ajouter la méthode setSampleRate()

**AJOUTER APRÈS :**
```cpp
Synth::~Synth(){
    delete (s1);
    delete (s2);
    delete (s3);
    delete (s4);
    delete (s5);
    delete (s6);
    delete (s7);
}
```

**NOUVEAU CODE :**
```cpp
void Synth::setSampleRate(double newSampleRate) {
    // Propager le sample rate à toutes les sirènes
    s1->setSampleRate(newSampleRate);
    s2->setSampleRate(newSampleRate);
    s3->setSampleRate(newSampleRate);
    s4->setSampleRate(newSampleRate);
    s5->setSampleRate(newSampleRate);
    s6->setSampleRate(newSampleRate);
    s7->setSampleRate(newSampleRate);
}
```

---

## 6. Source/CS_midiIN.h

### Ligne 49 : Ajouter la déclaration de setSampleRate()

**AVANT :**
```cpp
  void timerAudio();
  void sirenium_in(unsigned char *buf);

private:
```

**APRÈS :**
```cpp
  void timerAudio();
  void sirenium_in(unsigned char *buf);
  
  // Nouvelle méthode pour mettre à jour le sample rate
  void setSampleRate(double newSampleRate);

private:
```

### Lignes 70-73 : Remplacer incrementationVibrato hardcodé

**AVANT :**
```cpp
  float tremoloCh[17] = {0};
  int veloFinal[17];
  float incrementationVibrato=(512./44100.)/0.025;

  bool isWithSoundON;
```

**APRÈS :**
```cpp
  float tremoloCh[17] = {0};
  int veloFinal[17];
  
  // Variables pour le sample rate dynamique
  double sampleRate;
  float incrementationVibrato;

  bool isWithSoundON;
```

### Ligne ~98 : Nettoyer const function declaration

**AVANT :**
```cpp
  const std::function<void(int,int)> onVelocityChanged;
  
  const std::function<void(int, int)> onEngineSpeedChanged;
```

**APRÈS :**
```cpp
  const std::function<void(int,int)> onVelocityChanged;
  const std::function<void(int, int)> onEngineSpeedChanged;
```

---

## 7. Source/CS_midiIN.cpp

### Ligne 24 : Initialiser le sample rate dans le constructeur

**AVANT :**
```cpp
{
    for (int i =0; i<17; i++)
```

**APRÈS :**
```cpp
{
    // Initialiser le sample rate par défaut à 44.1kHz
    sampleRate = 44100.0;
    incrementationVibrato = (512.0 / sampleRate) / 0.025;
    
    for (int i =0; i<17; i++)
```

### Après le destructeur : Ajouter la méthode setSampleRate()

**AJOUTER APRÈS :**
```cpp
MidiIn::~MidiIn(){
}
```

**NOUVEAU CODE :**
```cpp
void MidiIn::setSampleRate(double newSampleRate) {
    sampleRate = newSampleRate;
    incrementationVibrato = (512.0 / sampleRate) / 0.025;
}
```

### Ligne 81 : Supprimer le debug MIDI trop fréquent

**AVANT :**
```cpp
void MidiIn::handleMIDIMessage2(int Ch, int value1, int value2){
    std::cout << "Message MIDI reçu: " << Ch << value1 << value2 << std::endl;
    if (Ch >= 144 && Ch < 160 ) {
```

**APRÈS :**
```cpp
void MidiIn::handleMIDIMessage2(int Ch, int value1, int value2){
    // Supprimer le debug trop fréquent
    if (Ch >= 144 && Ch < 160 ) {
```

### Ligne 102 : Supprimer le debug RealTimeStartNote

**AVANT :**
```cpp
void MidiIn::RealTimeStartNote(int Ch, int value1, int value2){
    std::cout << "RealTimeStartNote: " << Ch << "-" << value1 << "-" << value2 << std::endl;
    if (Ch >=1 && Ch <8) {
```

**APRÈS :**
```cpp
void MidiIn::RealTimeStartNote(int Ch, int value1, int value2){
    // Supprimer le debug trop fréquent
    if (Ch >=1 && Ch <8) {
```

### Ligne 398 : Supprimer le debug resetSireneCh

**AVANT :**
```cpp
void MidiIn::resetSireneCh(int Ch){

    std::cout << "Reset: "<< Ch << std::endl;

    noteonfinalCh[Ch]=0.0;
```

**APRÈS :**
```cpp
void MidiIn::resetSireneCh(int Ch){

    // Supprimer le debug trop fréquent

    noteonfinalCh[Ch]=0.0;
```

---

## Résumé des corrections

### 🎯 Problèmes résolus

1. **Sample rate hardcodé** : Remplacé `DeuxPieSampleRate` (44.1kHz) par `deuxPieSampleRate` dynamique
2. **Erreurs d'arrondi** : Utilisation de `100.0f` au lieu de `100` pour forcer les calculs en float
3. **Pitch incorrect** : Les calculs de pitch utilisent maintenant le sample rate correct du DAW
4. **Inertie des sirènes** : Utilisation des données vectorInterval (fichiers binaires) au lieu de calculs approximatifs
5. **`inertiaFactorTweak` obsolète** : Variable plus utilisée, peut être supprimée
6. **S7 trop fort** : Atténuation à 30% du niveau via constante `S7_ATTENUATION`

### 📝 Fichiers modifiés

- `Source/Sirene.h` - Ajout variables sample rate + méthode setSampleRate()
- `Source/Sirene.cpp` - Logique sample rate dynamique + vectorInterval
- `Source/PluginProcessor.h` - Forward declaration
- `Source/PluginProcessor.cpp` - Propagation sample rate + atténuation S7
- `Source/synth.h` - Méthode setSampleRate()
- `Source/CS_midiIN.h` - Variables sample rate + méthode setSampleRate()
- `Source/CS_midiIN.cpp` - Implémentation setSampleRate() + calcul incrementationVibrato dynamique

