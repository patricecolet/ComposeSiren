# Progression Architecture Multi-Plugin ComposeSiren 2.0

**Branche:** `feature/multi-plugin-architecture`  
**Date de démarrage:** 11 novembre 2025  
**Objectif:** Créer 6 plugins (Orchestra + 5 Solo S1/S3/S4/S5/S7)

---

## ✅ Phase 1: Restructuration et prototype S1 (TERMINÉ)

### Changements structurels
- ✅ Créé branche `feature/multi-plugin-architecture`
- ✅ Restructuré `Source/` en 3 dossiers:
  - `Common/` - Code partagé (13 fichiers)
  - `Orchestra/` - Plugin complet 7 sirènes
  - `Solo/` - 5 plugins individuels

### CMake
- ✅ Version passée à 2.0.0
- ✅ Créé bibliothèque statique `ComposeSiren_Common`
- ✅ Renommé plugin principal → `ComposeSiren_Orchestra`
- ✅ Fonction helper `add_siren_solo_plugin()` pour créer facilement de nouveaux plugins Solo

### Code Common adapté
- ✅ `Synth`: nouveau constructeur avec paramètres `soloMode` et `soloModel`
- ✅ Mode Solo: n'instancie que la sirène demandée (économie mémoire)
- ✅ Destructeur et `setSampleRate` : gèrent les pointeurs nullptr

### Plugin S1 Alto (prototype)
- ✅ `PluginProcessor_S1.cpp/h` : processeur simplifié 1 sirène
- ✅ `PluginEditor_S1.cpp/h` : interface avec affichage MIDI en temps réel
- ✅ Interface affiche:
  - LED Note On/Off (verte quand active)
  - Numéro de note MIDI + nom (ex: "60 (C4)")
  - Vélocité avec barre de progression
  - Liste des CC actifs (max 5 affichés)
- ✅ Configuration CMake ajoutée et activée

### Commit
```
7201567 - Phase 1: Restructuration multi-plugin - Architecture v2.0
```

---

## ✅ Phase 2: Tests et corrections (TERMINÉ)

### Réalisé
- ✅ Corrigé génération audio S1: `compute()` → `calculwave()`
- ✅ Créé liens symboliques pour compatibilité Xcode Projucer
- ✅ **Standalone** compilé avec succès
- ✅ **Audio Unit** compilé avec succès
- ✅ Architecture v2.0 validée sur macOS

### Solution de contournement
- Liens symboliques `Source/*.{cpp,h}` → `{Common/,Orchestra/}`
- Permet compilation avec projet Xcode existant
- Standalone et AU fonctionnent (VST3 échoue - bug JUCE connu)

### Commits
```
7201567 - Phase 1: Restructuration multi-plugin
e6d4fbc - Fix S1: Correction génération audio
a87f16a - Test: Validation compilation Orchestra
```

---

## 🔜 Phase 3: Duplication S3, S4, S5, S7

### Étapes
1. Copier `Solo/S1/` vers `Solo/S3/`, `/S4/`, `/S5/`, `/S7/`
2. Adapter les noms de classes et identifiants
3. Mettre à jour les titres d'affichage:
   - S3 Bass
   - S4 Tenor
   - S5 Soprano
   - S7 Piccolo
4. Décommenter les lignes correspondantes dans `CMakeLists.txt`
5. Tester la compilation des 5 plugins Solo

---

## 🔜 Phase 4: Documentation et packaging

### À faire
- [ ] Mettre à jour `README.md`
- [ ] Créer tableau comparatif Orchestra vs Solo
- [ ] Mettre à jour `docs/MIDI_CONTROL.md` (Solo = canal 1 unique)
- [ ] Créer packages d'installation
- [ ] Tests finaux dans différents DAW

---

## 📊 Statistiques

### Plugins
- **ComposeSiren Orchestra** : 7 sirènes + mixeur + reverb + limiter
- **ComposeSiren S1 Alto** : 1 sirène, interface MIDI simple
- **ComposeSiren S3 Bass** : À créer
- **ComposeSiren S4 Tenor** : À créer
- **ComposeSiren S5 Soprano** : À créer
- **ComposeSiren S7 Piccolo** : À créer

### Taille de code
- Code commun : ~13 fichiers partagés
- Plugin Orchestra : 2 fichiers (Processor + Editor)
- Plugin Solo S1 : 2 fichiers (Processor + Editor)

### Économie mémoire estimée (Solo vs Orchestra)
- Orchestra : charge ~22 fichiers de données
- Solo S1 : charge ~4 fichiers (S1 uniquement)
- **Gain : ~80% de mémoire économisée**

---

## 🎯 Objectifs v2.0

1. ✅ Architecture multi-plugin fonctionnelle
2. ✅ Plugin Orchestra conservé (rétrocompatibilité)
3. ⏳ 5 plugins Solo opérationnels
4. ⏳ Interface MIDI visuelle pour compositeurs
5. ⏳ Documentation complète
6. ⏳ Packages d'installation pour macOS/Linux/Windows

---

## 💡 Notes techniques

### Mode Solo vs Orchestra
```cpp
// Orchestra (défaut)
Synth* mySynth = new Synth();  // 7 sirènes

// Solo
Synth* mySynth = new Synth(true, "S1");  // 1 sirène uniquement
```

### Identifiants plugins
- Orchestra : `MvCs` (existant)
- S1 : `Cs01`
- S3 : `Cs03`
- S4 : `Cs04`
- S5 : `Cs05`
- S7 : `Cs07`

### Chemins ressources (inchangés)
- macOS : `/Library/Audio/Plug-ins/Mecanique Vivante/ComposeSiren/Resources/`
- Linux : `/usr/share/ComposeSiren/Resources/`
- Windows : `C:\Program Files\Common Files\Mecanique Vivante\ComposeSiren\Resources\`

