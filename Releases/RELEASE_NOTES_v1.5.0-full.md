# ComposeSiren v1.5.0 - Release complète

**Date:** 11 novembre 2025  
**Fichier:** ComposeSiren-v1.5.0-macOS.dmg  
**Taille:** 68 MB

## Contenu du package

Cette release contient **tous les formats de plugin disponibles** :

### 🎹 Plugins Audio
- **ComposeSiren.vst3** - Format VST3 (compatible Ableton Live, Reaper, Bitwig, etc.)
- **ComposeSiren.component** - Format Audio Unit (compatible Logic Pro, GarageBand, etc.)

### 🖥️ Application Standalone
- **ComposeSiren.app** - Application autonome (ne nécessite pas de DAW)

## Compatibilité

- **Systèmes:** macOS 10.13 (High Sierra) ou supérieur
- **Architectures:** Universal Binary (Intel x86_64 + Apple Silicon ARM64)
- **DAWs supportés:** Logic Pro, Ableton Live, Pro Tools, Reaper, Bitwig, GarageBand, et tous les hôtes compatibles VST3/AU

## Installation

1. **Monter le DMG** - Double-cliquer sur le fichier .dmg
2. **Installer les plugins:**
   - Copier `ComposeSiren.vst3` dans `~/Library/Audio/Plug-Ins/VST3/`
   - Copier `ComposeSiren.component` dans `~/Library/Audio/Plug-Ins/Components/`
   - Copier `ComposeSiren.app` dans `/Applications/` (optionnel)
3. **Relancer votre DAW** pour que les plugins soient détectés

## Compilation

Cette release a été compilée avec :
- **Xcode 16.3** (16E140)
- **JUCE Framework** (version incluse dans le projet)
- **Optimisation:** Release (-O3, LTO activé)
- **Standard C++:** C++20

## Notes techniques

- Tous les plugins sont signés pour exécution locale (ad-hoc signing)
- Les binaires sont universels (Intel + Apple Silicon)
- Dead code stripping activé pour réduire la taille
- Tous les fichiers de ressources sont inclus dans chaque bundle

## Différence avec v1.5.0-custom-mix

- Cette version contient **VST3 en plus** des autres formats
- Build complet avec toutes les cibles disponibles
- Compilé avec Xcode 16.3 (plus récent)

---

**Développé par:** Mécanique Vivante  
**Site web:** https://www.mecanique-vivante.com

