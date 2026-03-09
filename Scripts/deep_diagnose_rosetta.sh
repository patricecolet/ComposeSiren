#!/bin/bash
# Diagnostic approfondi pour comprendre pourquoi Rosetta est toujours demandé

DMG_FILE="$1"

if [ -z "$DMG_FILE" ]; then
    DMG_FILE=$(find build/Packaging -name "*.dmg" 2>/dev/null | head -1)
    if [ -z "$DMG_FILE" ]; then
        DMG_FILE=$(find Releases -name "*.dmg" 2>/dev/null | head -1)
    fi
fi

if [ -z "$DMG_FILE" ] || [ ! -f "$DMG_FILE" ]; then
    echo "❌ Aucun DMG trouvé"
    exit 1
fi

echo "🔍 Diagnostic approfondi du problème Rosetta"
echo "DMG: $DMG_FILE"
echo ""

TEMP_DIR="/tmp/deep_diagnose_$$"
mkdir -p "$TEMP_DIR"
MOUNT_POINT="$TEMP_DIR/mount"

# Monter le DMG
hdiutil attach "$DMG_FILE" -mountpoint "$MOUNT_POINT" -nobrowse -quiet 2>&1
PKG_FILE=$(find "$MOUNT_POINT" -name "*.pkg" | head -1)
UNINSTALL_APP=$(find "$MOUNT_POINT" -name "Uninstall*.app" | head -1)

echo "=== 1. Vérification de l'app Uninstall ==="
if [ -n "$UNINSTALL_APP" ]; then
    UNINSTALL_BINARY="$UNINSTALL_APP/Contents/MacOS/applet"
    if [ -f "$UNINSTALL_BINARY" ]; then
        echo "App Uninstall trouvée: $UNINSTALL_APP"
        echo "Architecture du binaire:"
        file "$UNINSTALL_BINARY" 2>&1
        echo ""
        ARCH=$(lipo -info "$UNINSTALL_BINARY" 2>&1)
        echo "lipo info: $ARCH"
        
        if ! echo "$ARCH" | grep -q "arm64"; then
            echo "⚠️  L'app Uninstall n'est pas universelle !"
            echo "   C'est probablement la cause du problème Rosetta"
        else
            echo "✅ L'app Uninstall est universelle"
        fi
    fi
else
    echo "ℹ️  Aucune app Uninstall trouvée"
fi

echo ""
echo "=== 2. Vérification du DMG lui-même ==="
echo "Type de fichier DMG:"
file "$DMG_FILE" 2>&1
echo ""

echo "=== 3. Vérification de la signature du DMG ==="
codesign -dv --verbose=4 "$DMG_FILE" 2>&1 | grep -E "Authority|Format|architecture|platform" | head -10
echo ""

if [ -n "$PKG_FILE" ]; then
    echo "=== 4. Vérification de la signature du PKG ==="
    codesign -dv --verbose=4 "$PKG_FILE" 2>&1 | grep -E "Authority|Format|architecture|platform" | head -10
    echo ""
    
    # Extraire le PKG
    EXTRACT_DIR="$TEMP_DIR/extract"
    mkdir -p "$EXTRACT_DIR"
    cd "$EXTRACT_DIR"
    xar -xf "$PKG_FILE" 2>&1 | head -3
    
    echo "=== 5. Vérification détaillée des PackageInfo ==="
    for PKG_INFO in *.pkg/PackageInfo; do
        if [ -f "$PKG_INFO" ]; then
            PKG_NAME=$(basename $(dirname "$PKG_INFO"))
            echo "$PKG_NAME:"
            # Afficher la ligne complète avec pkg-info
            grep "<pkg-info" "$PKG_INFO" | head -1
            echo ""
        fi
    done
    
    echo "=== 6. Vérification du Distribution.xml ==="
    if [ -f "Distribution" ]; then
        echo "Première ligne (installer-gui-script):"
        head -1 "Distribution"
        echo ""
        echo "Recherche de hostArchitectures:"
        grep -o 'hostArchitectures="[^"]*"' "Distribution" | head -1
    fi
fi

# Nettoyer
hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
rm -rf "$TEMP_DIR"

echo ""
echo "💡 Si tout est correct mais Rosetta est toujours demandé,"
echo "   le problème peut venir de:"
echo "   1. L'app Uninstall n'est pas universelle"
echo "   2. Le DMG n'est pas correctement signé pour arm64"
echo "   3. macOS Sonoma vérifie autre chose (notarisation, etc.)"

