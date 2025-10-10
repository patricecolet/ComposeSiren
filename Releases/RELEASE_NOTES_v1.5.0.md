# ComposeSiren v1.5.0 - Release Notes

**Branche** : `custom-mix`  
**Date** : 10 octobre 2025  
**Commit** : `078635d`

## 🎛️ Nouveautés principales

### Mixer 7 canaux intégré
- Contrôle individuel de **Volume** et **Pan** pour chaque sirène (S1-S7)
- Bouton **Reset** par canal (réinitialise tous les paramètres)
- Indicateurs **MIDI Note On/Off** en temps réel (voyants verts)
- Slider de volume **Master** avec couleur distinctive (bleu)

### Reverb globale intégrée
- **Room Size** : Taille de la réverbération
- **Damping** : Amortissement des hautes fréquences
- **Dry/Wet** : Balance signal sec/effet
- **Width** : Largeur stéréo de la reverb
- Activation/désactivation par bouton ou MIDI CC64

### Interface graphique modernisée
- Fond **gris foncé** élégant
- **En-tête** : "COMPOSE SIREN v1.5.0" + nom de branche "custom-mix"
- Sliders de reverb **horizontaux** pour une meilleure ergonomie
- Boutons de pan **plus grands** pour une manipulation précise
- Label simplifié pour la section reverb : **"Canal 16"**

### Contrôle MIDI complet
- **Canaux 1-7** : Contrôle individuel des sirènes
  - CC7 : Volume
  - CC10 : Pan
  - CC70 : Master Volume indépendant
  - CC121 : Reset du canal
- **Canal 16** : Contrôle de la reverb globale
  - CC64 : Enable (≥64 = ON)
  - CC65 : Room Size
  - CC66 : Dry/Wet
  - CC67 : Damping
  - CC68 : Width
  - CC121 : **Reset TOUTES les sirènes** 🔄

### Sauvegarde d'état
- Tous les paramètres du mixer et de la reverb sont sauvegardés
- Persistance dans les presets DAW et les projets
- Restauration automatique à l'ouverture

## 📦 Contenu du package

### macOS (47 MB)
- **ComposeSiren.app** : Application standalone
- **ComposeSiren.component** : Plugin Audio Unit

### ⚠️ Limitations connues
- **VST3 non disponible** : Le plugin VST3 ne compile pas à cause d'un bug JUCE avec macOS 15
  - APIs obsolètes : `CGWindowListCreateImage`, `CVDisplayLink*`
  - Correctif attendu dans une future version de JUCE
- **Build CMake** : Ne fonctionne pas avec macOS 15 (même bug)
  - Solution temporaire : Utiliser Xcode pour compiler

## 🔧 Installation

### Application Standalone
```bash
/Applications/ComposeSiren.app
```

### Plugin Audio Unit
```bash
# Utilisateur
~/Library/Audio/Plug-Ins/Components/ComposeSiren.component

# Système (tous les utilisateurs)
/Library/Audio/Plug-Ins/Components/ComposeSiren.component
```

Voir `INSTALLATION.txt` dans le DMG pour plus de détails.

## 🎯 Compatibilité

- **macOS** : 11.6.4 ou supérieur
- **Architecture** : Universal (Intel x86_64 + Apple Silicon arm64)
- **Testé avec** : Ableton Live

## 📝 Historique des commits

- `078635d` : Add v1.5.0 macOS release package (Standalone + AU)
- `bb86d72` : Document mixer/reverb features and MIDI CC mapping (v1.5.0)
- `500010f` : Add MIDI reset via CC121 (per channel or global on ch16)
- `c7bc15d` : Reduce Linux output gain to x50
- `d0e4c55` : Make pan knobs larger, master volume slider blue, all reverb horizontal sliders
- `f9db6bb` : Change background to dark grey
- `4ac9fe5` : Update header: COMPOSE SIREN v1.5.0, branch name, simplified reverb label
- `e7a53df` : Remove CC7 display, make pan knobs more visible, change reverb knobs to sliders
- `5bd8c9a` : Add MIDI Note On/Off indicators for sirens
- `b4f2e8c` : Keep existing CC7 volume control, bind UI to it, add note indicators

## 🐛 Bugs corrigés

- ✅ Fuite mémoire `IIRFilter` résolue
- ✅ Problème de lancement de l'app résolu
- ✅ Gain de sortie Linux réduit (x50 au lieu de x100)
- ✅ Sauvegarde d'état des paramètres mixer/reverb

## 🚀 Prochaines étapes

1. **Attendre fix JUCE** pour macOS 15 (VST3 + CMake)
2. **Créer installeur Windows** (VST3 + Standalone)
3. **Créer installeur Linux** (Standalone)
4. **Tests approfondis** avec différentes DAW

## 📧 Support

- **GitHub** : https://github.com/patricecolet/ComposeSiren
- **Branche** : `custom-mix`
- **Issues** : https://github.com/patricecolet/ComposeSiren/issues

---

© 2025 Mécanique Vivante

