#!/bin/bash
# Script pour corriger le problème Rosetta dans le DMG
# Le problème peut venir du DMG lui-même, du PKG, ou de l'app Uninstall

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

echo "🔧 Correction du problème Rosetta dans le DMG"
echo "DMG: $DMG_FILE"
echo ""

TEMP_DIR="/tmp/fix_dmg_rosetta_$$"
mkdir -p "$TEMP_DIR"
MOUNT_POINT="$TEMP_DIR/mount"

# Monter le DMG
hdiutil attach "$DMG_FILE" -mountpoint "$MOUNT_POINT" -nobrowse -quiet 2>&1
PKG_FILE=$(find "$MOUNT_POINT" -name "*.pkg" | head -1)
UNINSTALL_APP=$(find "$MOUNT_POINT" -name "Uninstall*.app" | head -1)

if [ -z "$PKG_FILE" ]; then
    echo "❌ Aucun PKG trouvé"
    hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

echo "✅ PKG trouvé: $PKG_FILE"

# Vérifier et corriger l'app Uninstall si elle existe
if [ -n "$UNINSTALL_APP" ]; then
    echo ""
    echo "=== Vérification de l'app Uninstall ==="
    UNINSTALL_BINARY="$UNINSTALL_APP/Contents/MacOS/applet"
    if [ -f "$UNINSTALL_BINARY" ]; then
        ARCH=$(lipo -info "$UNINSTALL_BINARY" 2>&1)
        echo "Architecture actuelle: $ARCH"
        
        if ! echo "$ARCH" | grep -q "arm64"; then
            echo "⚠️  L'app Uninstall n'est pas universelle, mais c'est normal pour osacompile"
            echo "   (osacompile crée généralement des apps universelles)"
        fi
    fi
fi

# Extraire le PKG
EXTRACT_DIR="$TEMP_DIR/extract"
mkdir -p "$EXTRACT_DIR"
cd "$EXTRACT_DIR"
xar -xf "$PKG_FILE" 2>&1 | head -3

echo ""
echo "=== Vérification du Distribution.xml ==="
if [ -f "Distribution" ]; then
    DIST_FORMAT=$(grep -o 'hostArchitectures="[^"]*"' "Distribution" 2>/dev/null | head -1)
    echo "Format actuel: $DIST_FORMAT"
    
    if ! echo "$DIST_FORMAT" | grep -q 'hostArchitectures="x86_64 arm64"'; then
        echo "🔧 Correction du format dans Distribution.xml..."
        # Remplacer les virgules par des espaces
        sed -i '' 's/hostArchitectures="x86_64,arm64"/hostArchitectures="x86_64 arm64"/g' "Distribution"
        sed -i '' 's/hostArchitectures="arm64,x86_64"/hostArchitectures="x86_64 arm64"/g' "Distribution"
        echo "✅ Format corrigé"
    else
        echo "✅ Format déjà correct"
    fi
else
    echo "❌ Distribution.xml non trouvé"
fi

echo ""
echo "=== Vérification des PackageInfo ==="
PKGINFO_FIXED=0
for PKG_INFO in *.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKG_NAME=$(basename $(dirname "$PKG_INFO"))
        PKGINFO_FORMAT=$(grep -o 'hostArchitectures="[^"]*"' "$PKG_INFO" 2>/dev/null | head -1)
        
        if ! echo "$PKGINFO_FORMAT" | grep -q 'hostArchitectures="x86_64 arm64"'; then
            echo "🔧 Correction du format dans $PKG_NAME/PackageInfo..."
            sed -i '' 's/hostArchitectures="x86_64,arm64"/hostArchitectures="x86_64 arm64"/g' "$PKG_INFO"
            sed -i '' 's/hostArchitectures="arm64,x86_64"/hostArchitectures="x86_64 arm64"/g' "$PKG_INFO"
            PKGINFO_FIXED=$((PKGINFO_FIXED + 1))
        fi
    fi
done

if [ $PKGINFO_FIXED -gt 0 ]; then
    echo "✅ $PKGINFO_FIXED PackageInfo corrigé(s)"
else
    echo "✅ Tous les PackageInfo sont déjà corrects"
fi

# Reconstruire le PKG
echo ""
echo "=== Reconstruction du PKG ==="
cd "$TEMP_DIR"
NEW_PKG_FILE="$TEMP_DIR/$(basename "$PKG_FILE")"

# Détacher le DMG pour pouvoir le modifier
hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null

# Reconstruire avec xar (plus fiable que productbuild pour les modifications)
cd "$EXTRACT_DIR"
xar -cf "$NEW_PKG_FILE" * 2>&1 | head -3

if [ -f "$NEW_PKG_FILE" ]; then
    echo "✅ PKG reconstruit"
    
    # Recréer le DMG
    echo ""
    echo "=== Recréation du DMG ==="
    DMG_CONTENT_DIR="$TEMP_DIR/dmgContent"
    mkdir -p "$DMG_CONTENT_DIR"
    
    # Copier le PKG corrigé
    cp "$NEW_PKG_FILE" "$DMG_CONTENT_DIR/"
    
    # Recréer le DMG temporaire
    TEMP_DMG="$TEMP_DIR/tmp.dmg"
    hdiutil create "$TEMP_DMG" -ov \
        -volname "$(basename "$DMG_FILE" .dmg)" \
        -fs HFS+ \
        -srcfolder "$DMG_CONTENT_DIR" 2>&1 | head -3
    
    # Convertir en format compressé
    NEW_DMG="${DMG_FILE%.dmg}_fixed.dmg"
    hdiutil convert "$TEMP_DMG" \
        -format UDZO \
        -o "$NEW_DMG" 2>&1 | head -3
    
    if [ -f "$NEW_DMG" ]; then
        echo "✅ DMG corrigé créé: $NEW_DMG"
        echo ""
        echo "💡 Testez ce nouveau DMG pour voir si Rosetta est toujours demandé"
    else
        echo "❌ Échec de la création du DMG"
    fi
    
    rm -f "$TEMP_DMG"
else
    echo "❌ Échec de la reconstruction du PKG"
fi

# Nettoyer
rm -rf "$TEMP_DIR"

