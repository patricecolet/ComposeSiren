# Correctif artefact de crossfade — ComposeSiren S7
## Session du 25 mai 2026

---

## Résumé

Artefact de clic audible lors des transitions entre notes (vibrato, glissando). Cause racine identifiée et corrigée par une combinaison de **correction des données** et **corrections de code**.

---

## 1. Cause racine : désordre d'indexation des harmoniques

### Le problème

Les fichiers `dataAmpSx` et `dataFreqSx` stockent les harmoniques par index (H0, H1, H2...). L'indexation ne correspond **pas** au même harmonique physique d'une note à l'autre dans les tables :

| Note (index table) | MIDI | H0 (fréq.) | Harmonique physique |
|---|---|---|---|
| 42 | 54 | 366 Hz | 2e harmonique |
| 43 | 55 | 194 Hz | Fondamentale |
| 44 | 56 | 412 Hz | 2e harmonique |
| 46 | 58 | 924 Hz | 4e harmonique ! |
| 47 | 59 | 618 Hz | 3e harmonique |
| 48 | 60 | 260 Hz | Fondamentale |

Le crossfade interpole les harmoniques par index :
```
freq_crossfade = freq_noteA[H5] * poids_A + freq_noteB[H5] * poids_B
```

Si H5 de la note A est à 1718 Hz (7e harmonique) et H5 de la note B est à 1552 Hz (6e harmonique), le crossfade crée des fréquences fantômes intermédiaires qui n'existent dans aucune des deux notes.

### Preuve

Test concluant : copier les données d'une seule note sur toutes les notes adjacentes élimine totalement le clic. Cela confirme que le problème est dans les données, pas dans le code de synthèse.

### Mapping MIDI → index table

| Sirènes | Multiplicateur | Formule | Exemple |
|---|---|---|---|
| S1, S2 | 5.0 | index = MIDI - 12 | MIDI 60 → index 48 |
| S3, S5, S6, S7 | 7.5 | index = MIDI - 12 | MIDI 60 → index 48 |
| S4 | 20/3 | index = MIDI - 12 | MIDI 60 → index 48 |

Formule complète : `midicent = round((69 + 12*log2(440 * 2^((MIDI-81)/12) * multiplicateur / 7.5 / 440)) * 100)`

---

## 2. Correction des données (appliquée sur S7)

### Étape 1 — Réindexation par harmonique physique

Pour chaque note, déterminer le numéro d'harmonique réel de chaque slot :
```
harmonique_physique = round(frequence_moyenne / f0)
```
où `f0 = 440 × 2^((index_table - 57) / 12)`

Réorganiser les slots : H0 = fondamentale (#1), H1 = 2e harmonique (#2), etc. En cas de collision (deux harmoniques originales mappant au même slot), garder celle avec la plus grande amplitude.

### Étape 2 — Forcer les fréquences aux harmoniques exactes

Pour chaque slot `h`, remplacer toutes les valeurs de fréquence (dans toutes les frames) par :
```
freq[note][frame][h] = f0 × (h + 1)
```

Les **amplitudes originales sont préservées**. Les slots vides (harmoniques absentes de la sirène physique) ont amplitude = 0 et fréquence = valeur théorique attendue.

**Résultat** : chaque transition entre notes adjacentes est exactement un demi-ton (ratio 1.0595) pour toutes les harmoniques, éliminant les fréquences fantômes du crossfade.

### Script Python complet

```python
import numpy as np

NOMBRE_DE_NOTE = 80
MAX_TAB = 1000
MAX_Partiel = 200

# Charger les données originales
amp = np.fromfile('dataAmpS7', dtype=np.float32).reshape(
    (NOMBRE_DE_NOTE, MAX_TAB, MAX_Partiel)).copy()
freq = np.fromfile('dataFreqS7', dtype=np.float32).reshape(
    (NOMBRE_DE_NOTE, MAX_TAB, MAX_Partiel)).copy()
dureTabs = np.fromfile('datadureTabsS7', dtype=np.float32).reshape(
    (NOMBRE_DE_NOTE, 3))

newAmp = np.zeros_like(amp)
newFreq = np.zeros_like(freq)

# Étape 1 : Réindexation
for note in range(NOMBRE_DE_NOTE):
    nbFrames = int(dureTabs[note][1])
    if nbFrames == 0:
        continue
    f0 = 440.0 * (2.0 ** ((note - 57) / 12.0))
    for h in range(MAX_Partiel):
        avgF = freq[note, :nbFrames, h].mean()
        if avgF <= 0:
            continue
        harmNum = max(1, round(avgF / f0))
        targetIdx = harmNum - 1
        if targetIdx >= MAX_Partiel:
            continue
        # En cas de collision, garder l'harmonique la plus forte
        existingA = newAmp[note, :nbFrames, targetIdx].mean()
        thisA = amp[note, :nbFrames, h].mean()
        if thisA > existingA:
            newAmp[note, :nbFrames, targetIdx] = amp[note, :nbFrames, h]
            newFreq[note, :nbFrames, targetIdx] = freq[note, :nbFrames, h]

# Étape 2 : Forcer les fréquences aux harmoniques exactes
for note in range(NOMBRE_DE_NOTE):
    nbFrames = int(dureTabs[note][1])
    if nbFrames == 0:
        continue
    f0 = 440.0 * (2.0 ** ((note - 57) / 12.0))
    maxH = min(MAX_Partiel, int(22000 / f0))
    for h in range(maxH):
        expectedF = f0 * (h + 1)
        newFreq[note, :nbFrames, h] = expectedF

# Sauvegarder
newAmp.tofile('dataAmpS7_corrected')
newFreq.tofile('dataFreqS7_corrected')
```

---

## 3. Corrections de code

### 3a. Sample rate dynamique (critique)

Le code original utilise `#define DeuxPieSampleRate (2.* M_PI / 44100)` hardcodé. À 48kHz, toutes les fréquences sont fausses.

**Fichiers modifiés** : `Sirene.h`, `Sirene.cpp`, `PluginProcessor.cpp`

```cpp
// Sirene.h — nouveaux membres privés
double deuxPieSampleRate = 2. * M_PI / 44100.;
double sampleRateRatio = 1.0;

// Sirene.h — nouvelle méthode publique
void setSampleRate(double sr);

// Sirene.cpp — implémentation
void Sirene::setSampleRate(double sr) {
    deuxPieSampleRate = 2. * M_PI / sr;
    sampleRateRatio = sr / 44100.;
}

// Sirene.cpp — setMidicent : remplacer DeuxPieSampleRate par deuxPieSampleRate
pitchSchift[noteInf] = (...) * deuxPieSampleRate;
pitchSchift[noteSup] = (...) * deuxPieSampleRate;

// Sirene.h — calculwave : scaler les durées de frames
if (countKInf == static_cast<int>(dureTabs[noteInf][0] * sampleRateRatio)) {
if (countKSup == static_cast<int>(dureTabs[noteSup][0] * sampleRateRatio)) {

// PluginProcessor.cpp — prepareToPlay
void SirenePlugAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    mySynth->s1->setSampleRate(sampleRate);
    // ... s2 à s7
}
```

### 3b. Phase wrapping

Remplacement du test mort `if (phaseInf[i] == 180.)` par un vrai wrapping en radians :
```cpp
while (phaseInf[i] >= 2. * M_PI)
    phaseInf[i] -= 2. * M_PI;
```

### 3c. Crossfade : fréquence quand amplitude ≈ 0

Quand une harmonique a une amplitude quasi-nulle dans une des deux notes, utiliser uniquement la fréquence de la note active (évite les sweeps de fréquence vers 0) :

```cpp
if (isCrossfade) {
    float ampInfVal = tabAmp[noteInf][countP[noteInf]][i];
    float ampSupVal = tabAmp[noteSup][countP[noteSup]][i];
    float freqInfVal = tabFreq[noteInf][countP[noteInf]][i] * pitchSchift[noteInf];
    float freqSupVal = tabFreq[noteSup][countP[noteSup]][i] * pitchSchift[noteSup];

    if (ampInfVal < 0.0001f && ampSupVal < 0.0001f) {
        phaseInf[i] += freqInfVal;
    } else if (ampInfVal < 0.0001f) {
        phaseInf[i] += freqSupVal;
    } else if (ampSupVal < 0.0001f) {
        phaseInf[i] += freqInfVal;
    } else {
        phaseInf[i] += freqInfVal * eloignementfreq / 100.
                     + freqSupVal * (100 - eloignementfreq) / 100.;
    }

    amp[i] = ampInfVal * eloignementfreq / 100.
           + ampSupVal * (100 - eloignementfreq) / 100.;
}
```

### 3d. setnote() sur le thread audio

Déplacement de `setnote()` depuis `timerCallback()` (thread GUI) vers `processBlock()` (thread audio) pour éliminer la race condition :

```cpp
// Dans processBlock, toutes les 44 samples :
if (sampleCountForMidiInTimer % 44 == 0) {
    mySynth->s1->setnote();
    // ... s2 à s7
}

// timerCallback vidé :
void SirenePlugAudioProcessor::timerCallback() {}
```

---

## 4. Corrections testées et rejetées

| Correction | Résultat | Raison |
|---|---|---|
| Reset countKInf/countKSup au changement de note | ❌ | Introduit une modulation rapide parasite |
| Interpolation linéaire entre frames temporelles | ❌ | Casse complètement le son (FM-like) |
| Lissage de fréquence par harmonique (freqz) | ❌ | Introduit des artefacts |
| Seuil d'amplitude (couper amp < 0.001) | ❌ | Modifie le timbre des autres sirènes |
| Lissage de pitchSchift | ❌ | Crée des chirps audibles |
| Lissage de eloignementfreq | Neutre | Aucun effet |
| Crossfade au niveau forme d'onde | Neutre | Ne corrige pas le clic |
| Interpolation des harmoniques manquantes depuis voisins | ❌ | Crée du contenu spectral artificiel |

---

## 5. Problème connu : limites de l'ambitus

Quand le vibrato pousse la note au-delà de la tessiture :
- **En bas** : `wavefinal = 0` si `noteEncour <= noteMin * 100` → coupure brutale
- **En haut** : `midiCentVoulue` clampé à `noteMidiCentMax`
- **S7** : noteMin=36 (MIDI 48), noteMidiCentMax=7900 (MIDI 91)

Un fondu progressif aux limites serait souhaitable.

---

## 6. Pour appliquer aux autres sirènes (S1-S6)

Le même script Python peut être appliqué à chaque sirène en adaptant les noms de fichiers (`dataAmpS1`, `dataFreqS1`, etc.). La formule `f0 = 440 * 2^((note - 57) / 12)` est identique pour toutes les sirènes. Vérifier que le résultat sonne correct sur toute la tessiture avant de valider.
