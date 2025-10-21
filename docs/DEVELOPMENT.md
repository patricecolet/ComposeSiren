# Guide de développement ComposeSiren

Documentation technique pour les développeurs contributeurs.

---

## 📋 Table des matières

- [Architecture](#architecture)
- [Gestion du sample rate](#gestion-du-sample-rate)
- [Algorithmes des sirènes](#algorithmes-des-sirènes)
- [Structure du code](#structure-du-code)
- [Correctifs importants](#correctifs-importants)
- [Contribution](#contribution)

---

## 🏗️ Architecture

### Technologies

- **Framework** : JUCE 7.x
- **Formats** : VST3, Audio Unit, Standalone
- **Langages** : C++17
- **Build** : CMake (Linux/Windows), Xcode (macOS)

### Composants principaux

```
PluginProcessor (SirenePlugAudioProcessor)
  ├── Synth
  │   ├── Sirene S1 (Alto)
  │   ├── Sirene S2 (Alto)
  │   ├── Sirene S3 (Bass)
  │   ├── Sirene S4 (Tenor)
  │   ├── Sirene S5 (Soprano)
  │   ├── Sirene S6 (Soprano)
  │   └── Sirene S7 (Piccolo)
  ├── MidiIn (CS_midiIN)
  │   └── Gestion des messages MIDI
  └── PluginEditor
      ├── Interface graphique
      ├── Mixer 7 canaux
      └── Reverb globale
```

---

## 🎚️ Gestion du sample rate

### Problème historique (avant v1.5.0)

Le code original utilisait un sample rate **hardcodé à 44.1kHz** :

```cpp
// AVANT (incorrect)
#define DeuxPieSampleRate (2.* M_PI / 44100)
```

Cela causait des problèmes de pitch incorrect lorsque le DAW utilisait un sample rate différent (48kHz, 96kHz, etc.).

### Solution implémentée

#### 1. Variables dynamiques dans `Sirene.h`

```cpp
// Variables pour le sample rate dynamique
double sampleRate;
double deuxPieSampleRate;

// Méthode pour mettre à jour le sample rate
void setSampleRate(double newSampleRate);
```

#### 2. Initialisation dans `Sirene.cpp`

```cpp
Sirene::Sirene(const std::string& str, const std::string& dataFilePath) :
name(str) {
  // Initialiser le sample rate par défaut à 44.1kHz
  sampleRate = 44100.0;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;
  // ...
}
```

#### 3. Mise à jour via `setSampleRate()`

```cpp
void Sirene::setSampleRate(double newSampleRate) {
  sampleRate = newSampleRate;
  deuxPieSampleRate = (2.0 * M_PI) / sampleRate;
  
  // Recalculer les pitchShift avec le nouveau sample rate
  if (midiCentVoulue > 0) {
    setMidicent(midiCentVoulue);
  }
}
```

#### 4. Propagation depuis `PluginProcessor::prepareToPlay()`

```cpp
void SirenePlugAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Propager le sample rate aux composants
    if (mySynth != nullptr) {
        mySynth->setSampleRate(sampleRate);
    }
    
    if (myMidiInHandler != nullptr) {
        myMidiInHandler->setSampleRate(sampleRate);
    }
}
```

### Résultat

✅ Le plugin fonctionne maintenant correctement à **n'importe quel sample rate** (44.1kHz, 48kHz, 96kHz, etc.)

---

## 🌊 Algorithmes des sirènes

### Principe de base

Les sirènes de Mécanique Vivante sont synthétisées par **resynthèse additive** à partir de données FFT pré-calculées.

### Données binaires

Chaque sirène (S1-S7) a 4 fichiers de données :

- **`dataAmpSX`** : Amplitudes des partiels (392 notes × 100 fenêtres × 200 partiels)
- **`dataFreqSX`** : Fréquences des partiels
- **`datadureTabsSX`** : Durées des fenêtres FFT
- **`dataVectorIntervalSX`** : Inertie mécanique des sirènes

### Inertie des sirènes

La vraie innovation est la **simulation de l'inertie mécanique** des sirènes physiques.

#### Fonction `setnote()` dans `Sirene.cpp`

```cpp
void Sirene::setnote() {
  SireneSpeedSlideState ouJeSuis = oujesuis();
  auto appliedFactor = coeffPicolo;
  
  int note = (int)((noteEncour-50)/100.);
  if (note < noteMin) note = noteMin;
  int baseNoteIndex = note - noteMin;
  
  // Appliquer l'inertie selon l'état de la sirène
  if (ouJeSuis == Montant) {
    noteEncour = noteEncour + (100.0f / (vectorInterval[baseNoteIndex + 294] * appliedFactor));
    if(noteEncour > noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  }
  else if (ouJeSuis == Descandant) {
    noteEncour = noteEncour - (100.0f / (vectorInterval[391 - baseNoteIndex] * appliedFactor));
    if(noteEncour < noteVoulueAvantSlide) noteEncour = noteVoulueAvantSlide;
  }
  // ... autres états (TonUpBefore, QuartUpBefore, etc.)
  
  setMidicent(noteEncour);
}
```

#### Pourquoi `100.0f` ?

Utiliser `100.0f` (float) au lieu de `100` (int) **force le calcul en virgule flottante**, évitant les erreurs d'arrondi dans les divisions.

```cpp
// INCORRECT : arrondi entier
noteEncour = noteEncour + (100 / (vectorInterval[...] * appliedFactor));

// CORRECT : calcul en float
noteEncour = noteEncour + (100.0f / (vectorInterval[...] * appliedFactor));
```

### États de la sirène

```cpp
enum SireneSpeedSlideState {
  jesuisrest,       // Au repos
  Montant,          // Montée continue
  Descandant,       // Descente continue
  TonUpBefore,      // Saut d'un ton (avant)
  DemiUpBefore,     // Saut d'un demi-ton (avant)
  QuartUpBefore,    // Glissando d'un quart de ton (montée)
  QuartDownBefore,  // Glissando d'un quart de ton (descente)
  QuartUpAfter,     // ...
  QuartDownAfter,   // ...
  Boucle            // Boucle sur la note
};
```

---

## 📁 Structure du code

### Source/

```
Source/
├── PluginProcessor.h/cpp    # Processeur audio principal
├── PluginEditor.h/cpp        # Interface graphique (mixer + reverb)
├── Sirene.h/cpp              # Classe Sirene (synthèse)
├── synth.h/cpp               # Gestion des 7 sirènes
├── CS_midiIN.h/cpp           # Gestion MIDI
├── mareverbe.h/cpp           # Reverb stéréo
├── comb.h/cpp                # Filtres comb (pour reverb)
├── allpass.h/cpp             # Filtres allpass (pour reverb)
└── parameters.h              # Constantes et paramètres
```

### Resources/

Les fichiers de données FFT binaires (non versionnés, générés offline) :

```
Resources/
├── dataAmpS1, dataAmpS3, ..., dataAmpS7
├── dataFreqS1, dataFreqS3, ..., dataFreqS7
├── datadureTabsS1, ..., datadureTabsS7
└── dataVectorIntervalS1, ..., dataVectorIntervalS7
```

---

## 🔧 Correctifs importants

### v1.5.0 (Octobre 2025)

#### 1. Sample rate dynamique
- ❌ **Avant** : Hardcodé à 44.1kHz
- ✅ **Après** : Dynamique via `setSampleRate()`

#### 2. Corrections d'arrondi
- ❌ **Avant** : Division entière (`100 / x`)
- ✅ **Après** : Division flottante (`100.0f / x`)

#### 3. Pan (CC10) sur canaux 1-7
- ❌ **Avant** : Pan non fonctionnel
- ✅ **Après** : Pan MIDI opérationnel avec sync UI temps réel

#### 4. Reverb (CC64-70) sur canal 16
- ❌ **Avant** : Reverb non contrôlable par MIDI
- ✅ **Après** : Tous les paramètres reverb accessibles via MIDI CC

#### 5. Atténuation S7 (Piccolo)
- ❌ **Avant** : S7 trop fort par rapport aux autres
- ✅ **Après** : Atténuation à 30% via `S7_ATTENUATION`

```cpp
// PluginProcessor.cpp
static const float S7_ATTENUATION = 0.3f;

// Dans processBlock()
channelLeft[sample] = /* ... */ + sampleS7 * mySynth->getPan(7,0) * S7_ATTENUATION;
channelRight[sample] = /* ... */ + sampleS7 * mySynth->getPan(7,1) * S7_ATTENUATION;
```

#### 6. Synchronisation MIDI → Interface
- ❌ **Avant** : L'interface ne reflétait pas les changements MIDI
- ✅ **Après** : Callbacks `onVelocityChanged`, `onEngineSpeedChanged`

---

## 🧪 Tests

### Tester le sample rate dynamique

```cpp
// Dans PluginProcessor.cpp
void SirenePlugAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    DBG("Sample rate: " << sampleRate << " Hz");
    // ...
}
```

Testez avec différents sample rates dans votre DAW :
- 44.1kHz (CD quality)
- 48kHz (vidéo standard)
- 96kHz (Hi-Res)

Le pitch doit rester **identique** à tous les sample rates.

### Tester les contrôles MIDI

Utilisez un contrôleur MIDI ou un plugin de test MIDI :

1. **Pan (CC10)** : Envoyer sur canaux 1-7, vérifier dans l'interface
2. **Reverb (CC64-70)** : Envoyer sur canal 16, écouter l'effet
3. **Reset (CC121)** : Vérifier que tous les paramètres reviennent aux valeurs par défaut

---

## 🤝 Contribution

### Environnement de développement

**macOS** :
```bash
# Installer Xcode Command Line Tools
xcode-select --install

# Cloner avec submodules
git clone --recursive https://github.com/patricecolet/ComposeSiren.git
cd ComposeSiren

# Ouvrir le projet Xcode
open Builds/MacOSX/ComposeSiren.xcodeproj
```

**Linux** :
```bash
# Installer les dépendances
sudo apt-get install cmake build-essential libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libfreetype-dev libasound2-dev

# Cloner et build
git clone --recursive https://github.com/patricecolet/ComposeSiren.git
cd ComposeSiren
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Workflow Git

```bash
# Créer une branche feature
git checkout -b feature/ma-nouvelle-fonctionnalite

# Faire vos modifications
# ...

# Commit avec message descriptif
git add .
git commit -m "feat: Ajout de XYZ

- Détail 1
- Détail 2"

# Pusher et créer une Pull Request
git push origin feature/ma-nouvelle-fonctionnalite
```

### Conventions de code

- **Style** : Suivre le style JUCE existant
- **Nommage** :
  - Classes : `PascalCase`
  - Méthodes : `camelCase`
  - Variables : `camelCase`
  - Constantes : `UPPER_CASE`
- **Commentaires** : En français ou anglais, selon le contexte

### Checklist avant PR

- [ ] Le code compile sans warnings
- [ ] Les modifications sont testées (macOS + Linux si possible)
- [ ] La documentation est mise à jour si nécessaire
- [ ] Pas de hardcoded paths ou credentials
- [ ] Le .gitignore est respecté (pas de build artifacts)

---

## 📚 Ressources

### JUCE Documentation
- [JUCE Tutorials](https://docs.juce.com/master/tutorial_new_projucer_project.html)
- [JUCE API Reference](https://docs.juce.com/master/index.html)
- [JUCE Forum](https://forum.juce.com/)

### Audio DSP
- [The Scientist and Engineer's Guide to Digital Signal Processing](http://www.dspguide.com/)
- [DAFX: Digital Audio Effects](https://www.dafx.de/)

### Mécanique Vivante
- [Site officiel](https://www.mecanique-vivante.com/)
- [Les Sirènes musicales](https://www.mecanique-vivante.com/en/the-song-of-the-sirens/the-musical-siren)

---

**Dernière mise à jour** : 21 Octobre 2025

