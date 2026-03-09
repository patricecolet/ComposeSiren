#!/bin/bash
# Script pour vérifier le format final du package après correction

DMG_FILE="$1"

if [ -z "$DMG_FILE" ]; then
    # Chercher dans build puis Releases
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

echo "🔍 Vérification finale du format dans: $DMG_FILE"
echo ""

TEMP_DIR="/tmp/verify_final_$$"
mkdir -p "$TEMP_DIR"
MOUNT_POINT="$TEMP_DIR/mount"

# Monter le DMG
hdiutil attach "$DMG_FILE" -mountpoint "$MOUNT_POINT" -nobrowse -quiet 2>&1
if [ $? -ne 0 ]; then
    echo "❌ Impossible de monter le DMG"
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Trouver le PKG
PKG_FILE=$(find "$MOUNT_POINT" -name "*.pkg" | head -1)
if [ -z "$PKG_FILE" ]; then
    echo "❌ Aucun PKG trouvé dans le DMG"
    hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Extraire
EXTRACT_DIR="$TEMP_DIR/extract"
mkdir -p "$EXTRACT_DIR"
xar -xf "$PKG_FILE" -C "$EXTRACT_DIR" 2>&1 | head -3

echo ""
echo "=== Distribution.xml ==="
if [ -f "$EXTRACT_DIR/Distribution" ]; then
    DIST_LINE=$(grep "hostArchitectures" "$EXTRACT_DIR/Distribution" | head -1)
    echo "$DIST_LINE"
    
    if echo "$DIST_LINE" | grep -q 'hostArchitectures="x86_64 arm64"'; then
        echo "✅ Format CORRECT (espaces)"
        DIST_OK=1
    elif echo "$DIST_LINE" | grep -q 'hostArchitectures="x86_64,arm64"'; then
        echo "❌ Format INCORRECT (virgules)"
        DIST_OK=0
    else
        echo "⚠️  Format inattendu"
        DIST_OK=0
    fi
else
    echo "❌ Distribution.xml non trouvé"
    DIST_OK=0
fi

echo ""
echo "=== PackageInfo (tous les sous-packages) ==="
PKGINFO_OK=1
for PKG_INFO in "$EXTRACT_DIR"/*.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKG_NAME=$(basename $(dirname "$PKG_INFO"))
        PKG_INFO_LINE=$(grep "hostArchitectures" "$PKG_INFO" | head -1)
        
        if echo "$PKG_INFO_LINE" | grep -q 'hostArchitectures="x86_64 arm64"'; then
            echo "✅ $PKG_NAME: Format correct (espaces)"
        elif echo "$PKG_INFO_LINE" | grep -q 'hostArchitectures="x86_64,arm64"'; then
            echo "❌ $PKG_NAME: Format incorrect (virgules)"
            PKGINFO_OK=0
        elif echo "$PKG_INFO_LINE" | grep -q "hostArchitectures"; then
            echo "⚠️  $PKG_NAME: Format inattendu"
            echo "   $PKG_INFO_LINE"
            PKGINFO_OK=0
        else
            echo "❌ $PKG_NAME: hostArchitectures manquant"
            PKGINFO_OK=0
        fi
    fi
done

# Nettoyer
hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
rm -rf "$TEMP_DIR"

echo ""
echo "============================================"
if [ $DIST_OK -eq 1 ] && [ $PKGINFO_OK -eq 1 ]; then
    echo "✅ FORMAT CORRECT - Le package devrait fonctionner sur Sonoma"
    echo ""
    echo "Le package est prêt à être testé. Il ne devrait PAS demander Rosetta."
else
    echo "❌ FORMAT INCORRECT - Des corrections sont nécessaires"
    echo ""
    if [ $DIST_OK -eq 0 ]; then
        echo "   - Distribution.xml a un format incorrect"
    fi
    if [ $PKGINFO_OK -eq 0 ]; then
        echo "   - Certains PackageInfo ont un format incorrect"
        echo ""
        echo "   Solution: Exécutez ./scripts/fix_pkginfo_after_build.sh"
    fi
fi
echo "============================================"

