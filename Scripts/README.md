# Scripts d'automatisation ComposeSiren

Ce dossier contient des scripts utilitaires pour automatiser le processus de build.

## 📄 Fichiers

### copy_icon.sh
Script shell pour copier automatiquement l'icône de l'application dans le bundle lors de la compilation.

**Usage** : Ajouter comme Build Phase dans Xcode (voir ICON_BUILD_PHASE.md)

### ICON_BUILD_PHASE.md
Instructions détaillées pour intégrer l'icône automatiquement dans les builds Xcode.

**3 méthodes disponibles** :
1. Manuelle via Xcode (recommandé)
2. Script Python automatique (expérimental)
3. Via Projucer (si vous utilisez Projucer)

### add_icon_build_phase.py
Script Python pour modifier automatiquement le projet Xcode et ajouter la Build Phase de copie d'icône.

**Usage** :
```bash
cd Builds/MacOSX
python3 ../../Scripts/add_icon_build_phase.py
```

⚠️ **Attention** : Crée un backup automatique avant modification

## 🚀 Démarrage rapide

La méthode la plus simple est d'ajouter manuellement la Build Phase dans Xcode :

1. Ouvrir `Builds/MacOSX/ComposeSiren.xcodeproj`
2. Target "ComposeSiren - Standalone Plugin" → Build Phases
3. Ajouter "New Run Script Phase"
4. Script : `"${PROJECT_DIR}/Scripts/copy_icon.sh"`
5. Compiler !

Voir `ICON_BUILD_PHASE.md` pour les détails complets.

