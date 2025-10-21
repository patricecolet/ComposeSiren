# Contrôle MIDI - ComposeSiren

Guide complet des contrôleurs MIDI supportés par ComposeSiren.

---

## 📋 Table des matières

- [Vue d'ensemble](#vue-densemble)
- [Canaux 1-7 (Sirènes individuelles)](#canaux-1-7-sirènes-individuelles)
- [Canal 16 (Reverb globale)](#canal-16-reverb-globale)
- [Notes MIDI](#notes-midi)
- [Exemples pratiques](#exemples-pratiques)

---

## 🎹 Vue d'ensemble

ComposeSiren répond aux messages MIDI sur **16 canaux** :

- **Canaux 1-7** : Contrôle individuel des 7 sirènes (S1-S7)
- **Canaux 8-15** : Non utilisés (réservés pour futures extensions)
- **Canal 16** : Reverb globale + Reset ALL

### Architecture MIDI

```
Canal MIDI 1  →  Sirène S1 (Alto)
Canal MIDI 2  →  Sirène S2 (Alto)
Canal MIDI 3  →  Sirène S3 (Bass)
Canal MIDI 4  →  Sirène S4 (Tenor)
Canal MIDI 5  →  Sirène S5 (Soprano)
Canal MIDI 6  →  Sirène S6 (Soprano)
Canal MIDI 7  →  Sirène S7 (Piccolo)
Canal MIDI 16 →  Reverb globale + Reset ALL
```

---

## 🎛️ Canaux 1-7 (Sirènes individuelles)

### CC7 - Volume individuel

**Implémentation** : `CS_midiIN.cpp` ligne 164-169

```
Plage : 0-127
  0   = Silence
  64  = Volume moyen
  127 = Volume maximum
```

**Formule** : `volumeFinal = velocity × (CC7/127) × (500/127)`

**Exemple** :
```
Canal 1, CC7 = 100  →  Sirène S1 à ~78% du volume
Canal 5, CC7 = 64   →  Sirène S5 à 50% du volume
```

---

### CC10 - Pan (Panoramique gauche/droite)

**Implémentation** : `PluginProcessor.cpp` ligne 182-185

```
Plage : 0-127
  0   = Complètement à gauche (L=100%, R=0%)
  64  = Centre (L=50%, R=50%)
  127 = Complètement à droite (L=0%, R=100%)
```

**Formule interne** : `pan = (CC10 / 127.0) - 0.5`  (résultat : -0.5 à +0.5)

**Exemple** :
```
Canal 3, CC10 = 0    →  Sirène S3 100% à gauche
Canal 3, CC10 = 64   →  Sirène S3 au centre
Canal 3, CC10 = 127  →  Sirène S3 100% à droite
```

**✨ Synchronisation UI** : L'interface graphique reflète les changements en temps réel.

---

### CC70 - Master Volume (indépendant de CC7)

**Implémentation** : `PluginProcessor.cpp` ligne 186-189

```
Plage : 0-127
  0   = Mute
  127 = Volume master maximum
```

**Différence avec CC7** :
- **CC7** : Volume MIDI standard, multiplicateur de la vélocité
- **CC70** : Volume master du mixer, indépendant de la vélocité

**Formule** : `masterVolume = CC70 / 127.0`

**Exemple** :
```
Canal 1, CC70 = 100  →  Master volume S1 à ~78%
Canal 7, CC70 = 0    →  Mute S7 (piccolo)
```

**💡 Utilisation** : Permet de contrôler le volume du mixer indépendamment de la vélocité MIDI.

---

### CC121 - Reset All Controllers

**Implémentation** : `CS_midiIN.cpp` ligne 148-150

```
Valeur : N'importe quelle (0-127)
Action : Réinitialise TOUS les paramètres du canal
```

**Paramètres réinitialisés** :
- Notes en cours
- Volume
- Pan (retour au centre)
- Contrôles de modulation
- État de la sirène

**Exemple** :
```
Canal 3, CC121 = 0   →  Reset complet de la Sirène S3
Canal 5, CC121 = 127 →  Reset complet de la Sirène S5
```

---

## 🌀 Canal 16 (Gain global + Reverb globale)

Le canal 16 contrôle le gain global de sortie et la reverb stéréo globale appliquée à toutes les sirènes.

### CC7 - Gain Global (dB→RMS)

**Implémentation** : `PluginProcessor.cpp` ligne 193-195, formule : `synth.cpp` ligne 423-437

```
Plage : 0-127
  100 = Gain nominal (0 dB, ×1.0)
  0   = -100 dB (quasi-silence, ×0.00001)
  120 = +20 dB (×10.0)
  127 = +27 dB (×22.4)
```

**Formule** : Identique à `[dbtorms~]` de PureData :
```cpp
gain = pow(10, (ccValue - 100) / 20)
```

**Exemples** :
```
Canal 16, CC7 = 80  →  -20 dB (division par 10)
Canal 16, CC7 = 100 →  0 dB (nominal, ×1.0)
Canal 16, CC7 = 120 →  +20 dB (×10.0)
```

**💡 Utilisation** : Contrôle le volume général de sortie du plugin avec une courbe logarithmique (dB). Permet d'atténuer ou de booster le signal final avant la sortie audio.

---

### CC64 - Enable Reverb (On/Off)

**Implémentation** : `PluginProcessor.cpp` ligne 202-204

```
Plage : 0-127
  0-63  = Reverb OFF
  64-127 = Reverb ON
```

**Exemple** :
```
Canal 16, CC64 = 0   →  Désactiver la reverb
Canal 16, CC64 = 127 →  Activer la reverb
```

---

### CC65 - Room Size (Taille de la salle)

**Implémentation** : `PluginProcessor.cpp` ligne 205-207

```
Plage : 0-127
  0   = Petite pièce (reverb courte)
  64  = Salle moyenne
  127 = Grande cathédrale (reverb longue)
```

**Formule** : `roomSize = CC65 / 127.0`

**Exemple** :
```
Canal 16, CC65 = 32  →  Petite salle (~25%)
Canal 16, CC65 = 96  →  Grande salle (~75%)
```

---

### CC66 - Dry/Wet (Balance signal sec/effet)

**Implémentation** : `PluginProcessor.cpp` ligne 208-214

```
Plage : 0-127
  0   = 100% Dry (signal original uniquement, pas d'effet)
  64  = 50% Dry / 50% Wet (balance égale)
  127 = 100% Wet (effet uniquement, pas de signal original)
```

**Formule** :
```cpp
dryWet = CC66 / 127.0
wet = dryWet
dry = 1.0 - dryWet
```

**Exemple** :
```
Canal 16, CC66 = 0   →  Aucun effet reverb (100% dry)
Canal 16, CC66 = 64  →  50% signal original + 50% reverb
Canal 16, CC66 = 127 →  100% reverb (pas de dry)
```

---

### CC67 - Damping (Amortissement hautes fréquences)

**Implémentation** : `PluginProcessor.cpp` ligne 215-217

```
Plage : 0-127
  0   = Pas d'amortissement (reverb brillante)
  64  = Amortissement moyen
  127 = Amortissement maximum (reverb sombre/mate)
```

**Formule** : `damping = CC67 / 127.0`

**Exemple** :
```
Canal 16, CC67 = 0   →  Reverb brillante (conserve les aigus)
Canal 16, CC67 = 127 →  Reverb sombre (atténue les aigus)
```

**💡 Astuce** : Augmenter le damping pour simuler une salle avec matériaux absorbants (tapis, rideaux).

---

### CC68 - Highpass Filter (Filtre passe-haut)

**Implémentation** : `PluginProcessor.cpp` ligne 218-223

```
Plage : 0-127
  0   = 20 Hz (pas de filtrage grave)
  64  = ~1010 Hz
  127 = 2000 Hz (coupe graves)
```

**Formule** : `freq = 20 + (CC68 / 127.0) × 1980`

**Exemple** :
```
Canal 16, CC68 = 0   →  HPF à 20Hz (passe tout)
Canal 16, CC68 = 64  →  HPF à ~1kHz (coupe graves)
Canal 16, CC68 = 127 →  HPF à 2kHz (reverb légère sans graves)
```

**💡 Utilisation** : Enlever les basses fréquences de la reverb pour éviter qu'elle ne devienne boueuse.

---

### CC69 - Lowpass Filter (Filtre passe-bas)

**Implémentation** : `PluginProcessor.cpp` ligne 224-229

```
Plage : 0-127
  0   = 2000 Hz (coupe aigus)
  64  = ~11000 Hz
  127 = 20000 Hz (passe tout)
```

**Formule** : `freq = 2000 + (CC69 / 127.0) × 18000`

**Exemple** :
```
Canal 16, CC69 = 0   →  LPF à 2kHz (reverb très sombre)
Canal 16, CC69 = 64  →  LPF à ~11kHz (coupe les aigus extrêmes)
Canal 16, CC69 = 127 →  LPF à 20kHz (passe tout)
```

**💡 Utilisation** : Combiner avec CC68 pour créer une reverb de type "téléphone" (HPF élevé + LPF bas).

---

### CC70 - Width (Largeur stéréo)

**Implémentation** : `PluginProcessor.cpp` ligne 230-232

```
Plage : 0-127
  0   = Mono (pas de différence L/R)
  64  = Stéréo moyen
  127 = Stéréo large maximum
```

**Formule** : `width = CC70 / 127.0`

**Exemple** :
```
Canal 16, CC70 = 0   →  Reverb mono (identique L+R)
Canal 16, CC70 = 64  →  Stéréo moyen
Canal 16, CC70 = 127 →  Stéréo très large
```

**💡 Astuce** : Utiliser Width=0 pour garder la reverb au centre et éviter qu'elle n'élargisse trop le mix.

---

### CC121 - Reset ALL Sirens (Reset global)

**Implémentation** : `PluginProcessor.cpp` ligne 193-201

```
Valeur : N'importe quelle (0-127)
Action : Réinitialise TOUTES les sirènes (canaux 1-7)
```

**Sirènes réinitialisées** : S1, S2, S3, S4, S5, S6, S7

**Exemple** :
```
Canal 16, CC121 = 0  →  Reset complet de toutes les sirènes
```

**⚠️ Important** : Ce reset global NE réinitialise PAS les paramètres de la reverb elle-même.

---

## 🎵 Notes MIDI

### Plage de notes par sirène

**Implémentation** : `Sirene.cpp` (variables `noteMin`, `noteMidiCentMax`)

| Sirène | Type    | Note Min | Note Max (approx) |
|--------|---------|----------|-------------------|
| S1     | Alto    | C1 (24)  | C7 (96)           |
| S2     | Alto    | C1 (24)  | C7 (96)           |
| S3     | Bass    | C1 (24)  | E6 (88)           |
| S4     | Tenor   | C1 (24)  | F6 (89)           |
| S5     | Soprano | C2 (36)  | B7 (107)          |
| S6     | Soprano | C2 (36)  | B7 (107)          |
| S7     | Piccolo | C2 (36)  | B7 (107)          |

### Note On / Note Off

**Implémentation** : `CS_midiIN.cpp` ligne 81-141

- **Note On** : Démarre la sirène (avec vélocité)
- **Note Off** : Arrête la sirène

**Velocity** : La vélocité (0-127) contrôle l'amplitude initiale de la sirène.

### Pitch Bend

**Implémentation** : `CS_midiIN.cpp` ligne 205-212

- **Plage** : -8192 à +8191 (14-bit)
- **Effet** : Glissement de hauteur (pitch bend)

---

## 💡 Exemples pratiques

### Scénario 1 : Mixer basique (3 sirènes)

```
# S1 (Alto) à gauche, volume moyen
Canal 1, CC7 = 80
Canal 1, CC10 = 32

# S3 (Bass) au centre, volume fort
Canal 3, CC7 = 110
Canal 3, CC10 = 64

# S5 (Soprano) à droite, volume faible
Canal 5, CC7 = 50
Canal 5, CC10 = 96
```

### Scénario 2 : Reverb de cathédrale

```
# Gain global nominal
Canal 16, CC7 = 100

# Activer reverb
Canal 16, CC64 = 127

# Grande salle
Canal 16, CC65 = 110

# 40% wet
Canal 16, CC66 = 51

# Damping moyen
Canal 16, CC67 = 64

# Couper les graves (HPF 500Hz)
Canal 16, CC68 = 38

# Conserver les aigus (LPF 15kHz)
Canal 16, CC69 = 105

# Stéréo large
Canal 16, CC70 = 100
```

### Scénario 3 : Reverb de salle sèche

```
Canal 16, CC64 = 127   # Enable
Canal 16, CC65 = 40    # Petite salle
Canal 16, CC66 = 30    # Peu de wet
Canal 16, CC67 = 90    # Damping élevé
Canal 16, CC68 = 20    # HPF bas
Canal 16, CC69 = 80    # LPF moyen
Canal 16, CC70 = 50    # Stéréo modéré
```

### Scénario 4 : Boost de volume global

```
# Augmenter le volume global de +10 dB
Canal 16, CC7 = 110

# Ou boost massif +20 dB (×10)
Canal 16, CC7 = 120

# Réduire de -10 dB
Canal 16, CC7 = 90
```

### Scénario 5 : Reset d'urgence

```
# Reset toutes les sirènes
Canal 16, CC121 = 0

# Puis réinitialiser individuellement si besoin
Canal 1, CC121 = 0
Canal 2, CC121 = 0
# etc.
```

---

## 🔧 Implémentation technique

### Fichiers sources

- **`PluginProcessor.cpp`** (ligne 170-235) : Traitement CC pour Pan, Master Volume, Reverb (canal 16)
- **`CS_midiIN.cpp`** (ligne 143-203) : Traitement CC pour canaux 1-7 (Volume, etc.)
- **`PluginEditor.cpp`** : Interface graphique (sliders, knobs) avec synchronisation temps réel

### Synchronisation MIDI ↔ Interface

Tous les changements MIDI sont reflétés instantanément dans l'interface graphique via des callbacks :
- `onVelocityChanged` (volume)
- `onEngineSpeedChanged` (pan et autres paramètres)

---

## 📚 Ressources

- **Documentation principale** : [README.md](../README.md)
- **Guide Raspberry Pi** : [RASPBERRY_PI.md](RASPBERRY_PI.md)
- **Guide développeur** : [DEVELOPMENT.md](DEVELOPMENT.md)

---

**Dernière mise à jour** : 21 Octobre 2025  
**Version** : 1.5.1 (Gain global dB→RMS)

