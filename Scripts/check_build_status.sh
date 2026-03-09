#!/bin/bash
# Script pour vérifier l'état du build après une erreur de notarisation

BUILD_DIR="build"
PACKAGING_DIR="${BUILD_DIR}/Packaging/ComposeSiren_Orchestra_Installer_artefacts"

echo "🔍 Vérification de l'état du build..."
echo ""

# Vérifier si le DMG existe
DMG_FILE=$(find "${PACKAGING_DIR}" -name "*.dmg" 2>/dev/null | head -1)
if [ -n "$DMG_FILE" ]; then
    echo "✅ DMG trouvé: $DMG_FILE"
    echo "   Taille: $(du -h "$DMG_FILE" | cut -f1)"
    echo ""
    
    # Vérifier la signature
    echo "🔐 Vérification de la signature:"
    codesign -dv --verbose=4 "$DMG_FILE" 2>&1 | head -5
    echo ""
    
    # Vérifier le format hostArchitectures
    echo "📦 Vérification du format hostArchitectures:"
    TEMP_DIR="/tmp/check_dmg_$$"
    mkdir -p "$TEMP_DIR"
    hdiutil attach "$DMG_FILE" -mountpoint "$TEMP_DIR" -nobrowse -quiet 2>/dev/null
    if [ -f "$TEMP_DIR"/*.pkg ]; then
        PKG_FILE=$(find "$TEMP_DIR" -name "*.pkg" | head -1)
        xar -xf "$PKG_FILE" -C /tmp/check_pkg_$$ 2>/dev/null
        if [ -f /tmp/check_pkg_$$/Distribution ]; then
            echo "   Format dans Distribution.xml:"
            grep "hostArchitectures" /tmp/check_pkg_$$/Distribution | head -1
            rm -rf /tmp/check_pkg_$$
        fi
    fi
    hdiutil detach "$TEMP_DIR" -force -quiet 2>/dev/null
    rm -rf "$TEMP_DIR"
    echo ""
else
    echo "❌ Aucun DMG trouvé dans ${PACKAGING_DIR}"
fi

# Vérifier les logs de notarisation
if [ -f "${PACKAGING_DIR}/notarize-submit-log.txt" ]; then
    echo "📋 Logs de soumission de notarisation:"
    cat "${PACKAGING_DIR}/notarize-submit-log.txt" | tail -20
    echo ""
fi

if [ -f "${PACKAGING_DIR}/notarize-log.txt" ]; then
    echo "📋 Logs de notarisation:"
    cat "${PACKAGING_DIR}/notarize-log.txt" | tail -20
    echo ""
fi

echo "💡 Solutions possibles:"
echo "   1. Si le DMG existe, vous pouvez l'utiliser même sans staple"
echo "   2. Pour réessayer le staple: xcrun stapler staple \"$DMG_FILE\""
echo "   3. Pour désactiver la notarisation: ENABLE_NOTARIZATION=OFF dans LocalConfig.cmake"

