#!/bin/bash
# Script pour extraire le PKG du DMG et le tester directement

DMG_FILE="$1"

if [ -z "$DMG_FILE" ]; then
    DMG_FILE=$(find build/Packaging -name "*.dmg" 2>/dev/null | head -1)
    if [ -z "$DMG_FILE" ]; then
        DMG_FILE=$(find Releases -name "*.dmg" 2>/dev/null | head -1)
    fi
fi

if [ -z "$DMG_FILE" ] || [ ! -f "$DMG_FILE" ]; then
    echo "❌ Aucun DMG trouvé"
    echo "Usage: $0 [chemin_vers_dmg]"
    exit 1
fi

echo "📦 Extraction du PKG depuis le DMG"
echo "DMG: $DMG_FILE"
echo ""

TEMP_DIR="/tmp/extract_pkg_$$"
mkdir -p "$TEMP_DIR"
MOUNT_POINT="$TEMP_DIR/mount"

# Monter le DMG
hdiutil attach "$DMG_FILE" -mountpoint "$MOUNT_POINT" -nobrowse -quiet 2>&1
PKG_FILE=$(find "$MOUNT_POINT" -name "*.pkg" | head -1)

if [ -z "$PKG_FILE" ]; then
    echo "❌ Aucun PKG trouvé dans le DMG"
    hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Copier le PKG dans le même répertoire que le DMG source
DMG_DIR=$(dirname "$DMG_FILE")
OUTPUT_PKG="$DMG_DIR/$(basename "$PKG_FILE")"

echo "📋 PKG trouvé: $(basename "$PKG_FILE")"
echo "📥 Copie vers: $OUTPUT_PKG"
cp "$PKG_FILE" "$OUTPUT_PKG"

# Détacher le DMG
hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
rm -rf "$TEMP_DIR"

echo ""
echo "✅ PKG extrait avec succès"
echo ""
echo "🔍 Vérification du PKG extrait:"
echo "   Type de PKG:"
file "$OUTPUT_PKG" 2>&1
echo ""

TEMP_EXTRACT="/tmp/check_pkg_$$"
mkdir -p "$TEMP_EXTRACT"
cd "$TEMP_EXTRACT"

# Vérifier si c'est un bundle ou un xar
if [ -d "$OUTPUT_PKG" ]; then
    # Bundle-style package
    echo "   Bundle-style package détecté"
    cp -R "$OUTPUT_PKG/Contents" . 2>&1 | head -1 > /dev/null
    if [ -f "Contents/distribution.dist" ] || [ -f "Contents/Distribution" ]; then
        DIST_FILE=$(find Contents -name "Distribution" -o -name "distribution.dist" | head -1)
        if [ -n "$DIST_FILE" ]; then
            cp "$DIST_FILE" "Distribution" 2>&1
        fi
    fi
    if [ -d "Contents/Packages" ]; then
        cp -R Contents/Packages/*.pkg . 2>&1 | head -1 > /dev/null
    fi
else
    # Flat package (xar)
    xar -xf "$OUTPUT_PKG" 2>&1 | head -1 > /dev/null
fi

if [ -f "Distribution" ] || [ -f "Contents/distribution.dist" ]; then
    echo "   ✅ Distribution.xml trouvé"
    echo "   hostArchitectures:"
    grep -o 'hostArchitectures="[^"]*"' "Distribution" | head -1
    echo ""
    echo "   PackageInfo:"
    PKGINFO_COUNT=0
    PKGINFO_WITH_ARCH=0
    for PKG_INFO in *.pkg/PackageInfo; do
        if [ -f "$PKG_INFO" ]; then
            PKGINFO_COUNT=$((PKGINFO_COUNT + 1))
            PKG_NAME=$(basename $(dirname "$PKG_INFO"))
            if grep -q 'hostArchitectures=' "$PKG_INFO"; then
                PKGINFO_WITH_ARCH=$((PKGINFO_WITH_ARCH + 1))
                echo "   ✅ $PKG_NAME: $(grep -o 'hostArchitectures="[^"]*"' "$PKG_INFO" | head -1)"
            else
                echo "   ❌ $PKG_NAME: pas d'attribut hostArchitectures"
            fi
        fi
    done
    echo ""
    echo "   Résumé: $PKGINFO_WITH_ARCH/$PKGINFO_COUNT PackageInfo avec hostArchitectures"
fi

cd - > /dev/null
rm -rf "$TEMP_EXTRACT"

echo ""
echo "🧪 Test:"
echo "   Double-cliquez sur le PKG pour tester:"
echo "   $OUTPUT_PKG"
echo ""
echo "   Si Rosetta est demandé avec le PKG seul, le problème vient du PKG"
echo "   Si Rosetta n'est PAS demandé avec le PKG seul, le problème vient du DMG"

