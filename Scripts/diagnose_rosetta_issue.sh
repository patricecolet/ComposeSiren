#!/bin/bash
# Script de diagnostic complet pour comprendre pourquoi Rosetta est demandé

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

echo "🔍 Diagnostic complet du problème Rosetta"
echo "DMG: $DMG_FILE"
echo ""

TEMP_DIR="/tmp/diagnose_rosetta_$$"
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

# Extraire
EXTRACT_DIR="$TEMP_DIR/extract"
mkdir -p "$EXTRACT_DIR"
xar -xf "$PKG_FILE" -C "$EXTRACT_DIR" 2>&1 | head -3

echo "=== 1. Distribution.xml ==="
if [ -f "$EXTRACT_DIR/Distribution" ]; then
    echo "Ligne installer-gui-script:"
    grep "installer-gui-script" "$EXTRACT_DIR/Distribution" | head -1
    echo ""
    echo "hostArchitectures dans installer-gui-script:"
    grep -o 'hostArchitectures="[^"]*"' "$EXTRACT_DIR/Distribution" | head -1
    echo ""
    echo "Balise <options>:"
    grep "<options" "$EXTRACT_DIR/Distribution"
    echo ""
else
    echo "❌ Distribution.xml non trouvé"
fi

echo ""
echo "=== 2. PackageInfo (tous) ==="
for PKG_INFO in "$EXTRACT_DIR"/*.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKG_NAME=$(basename $(dirname "$PKG_INFO"))
        echo "$PKG_NAME:"
        grep -o 'hostArchitectures="[^"]*"' "$PKG_INFO" | head -1
    fi
done

echo ""
echo "=== 3. Vérification des binaires dans les packages ==="
for PKG_DIR in "$EXTRACT_DIR"/*.pkg; do
    if [ -d "$PKG_DIR" ]; then
        PKG_NAME=$(basename "$PKG_DIR")
        echo "$PKG_NAME:"
        
        # Extraire le Payload
        if [ -f "$PKG_DIR/Payload" ]; then
            PAYLOAD_DIR="$TEMP_DIR/payload_${PKG_NAME}"
            mkdir -p "$PAYLOAD_DIR"
            cd "$PAYLOAD_DIR"
            cat "$PKG_DIR/Payload" | gunzip | cpio -i 2>&1 | head -1 > /dev/null
            
            # Chercher les binaires
            BINARIES=$(find . -type f -perm +111 -path "*/MacOS/*" 2>/dev/null | head -3)
            if [ -n "$BINARIES" ]; then
                for BIN in $BINARIES; do
                    echo "  $(basename $BIN):"
                    lipo -info "$BIN" 2>&1 | sed 's/^/    /'
                done
            fi
            
            cd - > /dev/null
            rm -rf "$PAYLOAD_DIR"
        fi
    fi
done

echo ""
echo "=== 4. Vérification de la signature ==="
codesign -dv --verbose=4 "$PKG_FILE" 2>&1 | grep -E "Authority|Format|architecture" | head -5

echo ""
echo "=== 5. Analyse ==="
DIST_FORMAT=$(grep -o 'hostArchitectures="[^"]*"' "$EXTRACT_DIR/Distribution" 2>/dev/null | head -1)
if echo "$DIST_FORMAT" | grep -q 'hostArchitectures="x86_64 arm64"'; then
    echo "✅ Distribution.xml: Format correct (espaces)"
else
    echo "❌ Distribution.xml: Format incorrect ou manquant"
fi

PKGINFO_COUNT=0
PKGINFO_CORRECT=0
for PKG_INFO in "$EXTRACT_DIR"/*.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKGINFO_COUNT=$((PKGINFO_COUNT + 1))
        PKGINFO_FORMAT=$(grep -o 'hostArchitectures="[^"]*"' "$PKG_INFO" 2>/dev/null | head -1)
        if echo "$PKGINFO_FORMAT" | grep -q 'hostArchitectures="x86_64 arm64"'; then
            PKGINFO_CORRECT=$((PKGINFO_CORRECT + 1))
        fi
    fi
done

echo "PackageInfo: $PKGINFO_CORRECT/$PKGINFO_COUNT avec format correct"

# Nettoyer
hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null
rm -rf "$TEMP_DIR"

echo ""
echo "💡 Si le format est correct mais Rosetta est toujours demandé,"
echo "   le problème peut venir d'un autre composant (helper, script, etc.)"

