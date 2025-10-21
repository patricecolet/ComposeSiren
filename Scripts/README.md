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

### setup_codesigning.sh
Script interactif pour configurer la signature de code et la notarisation macOS.

**Usage** :
```bash
./Scripts/setup_codesigning.sh
```

Ce script vous guide pour :
- ✅ Détecter automatiquement vos certificats Developer ID
- ✅ Créer `LocalConfig.cmake` avec la configuration
- ✅ Configurer la notarisation Apple
- ✅ Produire un DMG installable sans avertissement

**Documentation complète** : Voir [../SIGNATURE_ET_NOTARISATION.md](../SIGNATURE_ET_NOTARISATION.md)

## 🚀 Démarrage rapide

### Build avec icône (Xcode)

La méthode la plus simple est d'ajouter manuellement la Build Phase dans Xcode :

1. Ouvrir `Builds/MacOSX/ComposeSiren.xcodeproj`
2. Target "ComposeSiren - Standalone Plugin" → Build Phases
3. Ajouter "New Run Script Phase"
4. Script : `"${PROJECT_DIR}/Scripts/copy_icon.sh"`
5. Compiler !

Voir `ICON_BUILD_PHASE.md` pour les détails complets.

### Build signé et notarisé (CMake)

Pour produire un DMG installable sans avertissement macOS :

1. Exécuter `./Scripts/setup_codesigning.sh` pour configurer
2. Builder : `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
3. Le DMG signé sera dans `build/Packaging/ComposeSiren_Installer_artefacts/`

Voir [../SIGNATURE_ET_NOTARISATION.md](../SIGNATURE_ET_NOTARISATION.md) pour les détails.


