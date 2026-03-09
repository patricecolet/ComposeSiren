#!/bin/bash
# Script pour vérifier le format hostArchitectures dans un DMG

DMG_FILE="$1"

if [ -z "$DMG_FILE" ]; then
    # Chercher automatiquement
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

echo "🔍 Vérification du format dans: $DMG_FILE"
echo ""

TEMP_DIR="/tmp/check_pkg_format_$$"
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

echo "📦 PKG trouvé: $(basename "$PKG_FILE")"
echo ""

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
    elif echo "$DIST_LINE" | grep -q 'hostArchitectures="x86_64,arm64"'; then
        echo "❌ Format INCORRECT (virgules) - C'est le problème !"
    elif echo "$DIST_LINE" | grep -q "hostArchitectures"; then
        echo "⚠️  Format présent mais inattendu"
        echo "$DIST_LINE"
    else
        echo "❌ hostArchitectures manquant !"
    fi
else
    echo "❌ Distribution.xml non trouvé"
fi

echo ""
echo "=== PackageInfo (tous les sous-packages) ==="
for PKG_INFO in "$EXTRACT_DIR"/*.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKG_NAME=$(basename $(dirname "$PKG_INFO"))
        PKG_INFO_LINE=$(grep "hostArchitectures" "$PKG_INFO" | head -1)
        echo "$PKG_NAME:"
        echo "  $PKG_INFO_LINE"
        
        if echo "$PKG_INFO_LINE" | grep -q 'hostArchitectures="x86_64 arm64"'; then
            echo "  ✅ Format correct"
        elif echo "$PKG_INFO_LINE" | grep -q 'hostArchitectures="x86_64,arm64"'; then
            echo "  ❌ Format incorrect (virgules)"
        fi
        echo ""
    fi
done

# Nettoyer
hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
rm -rf "$TEMP_DIR"

echo "💡 Si le format est incorrect, le rebuild n'a pas utilisé les templates corrigés."
echo "   Vérifiez que les fichiers .in dans Packaging/Apple/ ont bien été modifiés."

