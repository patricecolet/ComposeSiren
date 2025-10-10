# ComposeSiren v1.5.0 - Release Notes

![ComposeSiren Icon](https://github.com/patricecolet/ComposeSiren/releases/download/v1.5.0/Icon_1024.png)

**Branche** : `custom-mix`  
**Date** : 10 octobre 2025  
**Commit** : `c80dca1`

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
- **Icône de l'app** : Picto Siren haute résolution

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
- **ComposeSiren-v1.5.0-custom-mix-macOS.dmg** : Package complet
  - ComposeSiren.app : Application standalone
  - ComposeSiren.component : Plugin Audio Unit
  - INSTALLATION.txt : Instructions d'installation

### Icônes haute résolution
- **ComposeSiren.icns** (518 KB) : Format macOS natif multi-résolutions
- **Icon_1024.png** (8.9 MB) : Version PNG 1024x1024 pour développeurs

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

- `c80dca1` : Add automation scripts for app icon integration
- `9b90bf7` : Add high-resolution app icon based on Picto_Siren
- `878311c` : Add release notes for v1.5.0
- `078635d` : Add v1.5.0 macOS release package (Standalone + AU)
- `bb86d72` : Document mixer/reverb features and MIDI CC mapping (v1.5.0)
- `500010f` : Add MIDI reset via CC121 (per channel or global on ch16)
- `c7bc15d` : Reduce Linux output gain to x50
- Plus d'une dizaine de commits pour l'interface, mixer et reverb...

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

