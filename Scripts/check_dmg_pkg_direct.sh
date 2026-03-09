#!/bin/bash
# Vérification directe du PKG dans le DMG sans extraction

DMG_FILE="$1"

if [ -z "$DMG_FILE" ]; then
    DMG_FILE=$(find Releases -name "*.dmg" | head -1)
fi

if [ -z "$DMG_FILE" ] || [ ! -f "$DMG_FILE" ]; then
    echo "❌ Aucun DMG trouvé"
    exit 1
fi

echo "🔍 Vérification directe du PKG dans le DMG"
echo "DMG: $DMG_FILE"
echo ""

TEMP_DIR="/tmp/check_dmg_direct_$$"
mkdir -p "$TEMP_DIR"
MOUNT_POINT="$TEMP_DIR/mount"

# Monter le DMG
hdiutil attach "$DMG_FILE" -mountpoint "$MOUNT_POINT" -nobrowse -quiet 2>&1
PKG_FILE=$(find "$MOUNT_POINT" -name "*.pkg" | head -1)

if [ -z "$PKG_FILE" ]; then
    echo "❌ Aucun PKG trouvé"
    hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

echo "📦 PKG trouvé: $PKG_FILE"
echo ""

# Vérifier le type
PKG_TYPE=$(file "$PKG_FILE" 2>&1)
echo "Type: $PKG_TYPE"
echo ""

# Vérifier la signature
echo "=== Signature du PKG ==="
codesign -dv --verbose=4 "$PKG_FILE" 2>&1 | grep -E "Authority|Format|architecture|platform|Identifier" | head -10
echo ""

# Extraire selon le type
EXTRACT_DIR="$TEMP_DIR/extract"
mkdir -p "$EXTRACT_DIR"
cd "$EXTRACT_DIR"

if [ -d "$PKG_FILE" ]; then
    # Bundle-style
    echo "📂 Bundle-style package, copie du contenu..."
    cp -R "$PKG_FILE/Contents" . 2>&1 | head -1 > /dev/null
    
    # Chercher Distribution
    if [ -f "Contents/distribution.dist" ]; then
        DIST_FILE="Contents/distribution.dist"
    elif [ -f "Contents/Distribution" ]; then
        DIST_FILE="Contents/Distribution"
    fi
    
    # Chercher les sous-packages
    if [ -d "Contents/Packages" ]; then
        echo "📦 Sous-packages trouvés dans Contents/Packages"
        for SUB_PKG in Contents/Packages/*.pkg; do
            if [ -d "$SUB_PKG" ]; then
                PKG_NAME=$(basename "$SUB_PKG")
                echo "   - $PKG_NAME"
                if [ -f "$SUB_PKG/PackageInfo" ]; then
                    echo "     PackageInfo: $(grep -o 'hostArchitectures="[^"]*"' "$SUB_PKG/PackageInfo" 2>/dev/null | head -1 || echo 'non trouvé')"
                fi
            fi
        done
    fi
else
    # Flat package (xar)
    echo "📦 Flat package (xar), extraction..."
    xar -xf "$PKG_FILE" 2>&1 | head -3
    DIST_FILE="Distribution"
fi

echo ""
echo "=== Distribution.xml ==="
if [ -n "$DIST_FILE" ] && [ -f "$DIST_FILE" ]; then
    echo "Fichier: $DIST_FILE"
    echo "Première ligne:"
    head -1 "$DIST_FILE"
    echo ""
    echo "hostArchitectures:"
    grep -o 'hostArchitectures="[^"]*"' "$DIST_FILE" | head -1
else
    echo "❌ Distribution.xml non trouvé"
fi

# Nettoyer
hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
rm -rf "$TEMP_DIR"

echo ""
echo "💡 Si hostArchitectures est correct mais Rosetta est demandé,"
echo "   vérifiez les binaires dans les sous-packages"



