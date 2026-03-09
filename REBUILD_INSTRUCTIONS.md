# Instructions pour le rebuild complet

## ✅ Corrections appliquées

Les templates ont été corrigés pour utiliser le format avec **espaces** :
- `hostArchitectures="x86_64 arm64"` (format correct pour Sonoma)

## Commandes à exécuter

### Option 1 : Utiliser le script de build (recommandé)

```bash
cd /Users/patricecolet/repo/ComposeSiren-fixed
./scripts/build_release_macos.sh 2.0.1
```

### Option 2 : Build manuel étape par étape

```bash
cd /Users/patricecolet/repo/ComposeSiren-fixed

# 1. Nettoyer les anciens builds
rm -rf build

# 2. Configuration CMake
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15"

# 3. Build complet (compilation + packaging)
cmake --build build --config Release

# 4. Vérifier que le DMG a été créé
find build/Packaging/ComposeSiren_Orchestra_Installer_artefacts -name "*.dmg"
```

## Vérification après build

Une fois le build terminé, vérifiez le format du package :

```bash
# Monter le DMG généré
DMG_FILE=$(find build/Packaging/ComposeSiren_Orchestra_Installer_artefacts -name "*.dmg" | head -1)
hdiutil attach "$DMG_FILE" -mountpoint /tmp/check_dmg

# Extraire et vérifier
xar -xf /tmp/check_dmg/*.pkg -C /tmp/check_pkg
grep "hostArchitectures" /tmp/check_pkg/Distribution

# Devrait afficher : hostArchitectures="x86_64 arm64" (avec espaces)

# Nettoyer
hdiutil detach /tmp/check_dmg
rm -rf /tmp/check_pkg
```

## Résultat attendu

Le package généré devrait :
- ✅ Avoir `hostArchitectures="x86_64 arm64"` dans Distribution.xml
- ✅ Ne pas demander Rosetta sur macOS Sonoma
- ✅ Être valide et ouvrable sans erreur

## Note

Le build peut prendre plusieurs minutes selon votre machine.
Le packaging se fait automatiquement après la compilation.

