#!/bin/bash
# Script pour builder, notariser et préparer une release macOS
# Usage: ./scripts/build_release_macos.sh [version]

set -e  # Arrêter en cas d'erreur

VERSION=${1:-"1.5.1"}
BUILD_DIR="build"
PACKAGING_DIR="${BUILD_DIR}/Packaging/ComposeSiren_Orchestra_Installer_artefacts"

echo "============================================"
echo "🔨 Build et Release macOS ComposeSiren"
echo "Version: ${VERSION}"
echo "============================================"
echo ""

# Vérifier qu'on est sur macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ Ce script ne fonctionne que sur macOS"
    exit 1
fi

# Vérifier que LocalConfig.cmake existe
if [ ! -f "LocalConfig.cmake" ]; then
    echo "❌ LocalConfig.cmake n'existe pas"
    echo "   Exécutez: ./scripts/setup_codesigning.sh"
    exit 1
fi

# Vérifier que la notarisation est activée
if ! grep -q "ENABLE_NOTARIZATION ON" LocalConfig.cmake; then
    echo "⚠️  ATTENTION: ENABLE_NOTARIZATION n'est pas ON dans LocalConfig.cmake"
    read -p "Continuer quand même ? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Nettoyer les anciens builds
echo "🧹 Nettoyage des anciens builds..."
rm -rf "${BUILD_DIR}"
echo "✅ Nettoyage terminé"
echo ""

# Configuration CMake
echo "⚙️  Configuration CMake..."
cmake -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15"
echo "✅ Configuration terminée"
echo ""

# Build
echo "🔨 Compilation (cela peut prendre du temps)..."
cmake --build "${BUILD_DIR}" --config Release
echo "✅ Compilation terminée"
echo ""

# Vérifier que le DMG a été créé
DMG_FILE=$(find "${PACKAGING_DIR}" -name "*.dmg" 2>/dev/null | head -1)
if [ -z "$DMG_FILE" ]; then
    echo "❌ Aucun DMG trouvé dans ${PACKAGING_DIR}"
    exit 1
fi

echo "📦 DMG trouvé: ${DMG_FILE}"
echo ""

# Vérifier la notarisation du DMG
echo "🔍 Vérification de la notarisation..."
if spctl --assess --verbose "${DMG_FILE}" 2>&1 | grep -q "accepted"; then
    echo "✅ DMG notarisé et accepté par Gatekeeper"
    NOTARIZED=true
else
    echo "⚠️  DMG non notarisé ou rejeté par Gatekeeper"
    echo "   Vérification détaillée:"
    spctl --assess --verbose "${DMG_FILE}" || true
    NOTARIZED=false
fi
echo ""

# Vérifier la signature
echo "🔍 Vérification de la signature..."
codesign -dv --verbose=4 "${DMG_FILE}" 2>&1 | head -20
echo ""

# Préparer la release
RELEASES_DIR="Releases"
VERSION_DIR="${RELEASES_DIR}/v${VERSION}-macOS"
mkdir -p "${VERSION_DIR}"

echo "📝 Préparation de la release..."
DMG_NAME="ComposeSiren-v${VERSION}-macOS.dmg"
RELEASE_DMG="${VERSION_DIR}/${DMG_NAME}"

cp "${DMG_FILE}" "${RELEASE_DMG}"
echo "✅ DMG copié vers: ${RELEASE_DMG}"
echo ""

# Vérification finale
echo "============================================"
echo "✅ Build terminé avec succès!"
echo "============================================"
echo ""
echo "📦 Fichier release: ${RELEASE_DMG}"
echo "📊 Taille: $(du -h "${RELEASE_DMG}" | cut -f1)"
echo ""

if [ "$NOTARIZED" = true ]; then
    echo "✅ Status: Notarisé et prêt pour distribution"
else
    echo "⚠️  Status: Non notarisé - des problèmes peuvent survenir sur macOS Sonoma"
fi

echo ""
echo "Pour tester le DMG sur un autre Mac:"
echo "  spctl --assess --verbose \"${RELEASE_DMG}\""
echo ""
echo "Pour mettre à jour le README:"
echo "  Éditez README.md et changez la version à ${VERSION}"
echo ""

