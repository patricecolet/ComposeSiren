#!/bin/bash
# Script pour vérifier les architectures des binaires dans le PKG

PKG_FILE="$1"

if [ -z "$PKG_FILE" ]; then
    PKG_FILE=$(find Releases -name "*.pkg" | head -1)
    if [ -z "$PKG_FILE" ]; then
        PKG_FILE=$(find build/Packaging -name "*.pkg" | head -1)
    fi
fi

if [ -z "$PKG_FILE" ] || [ ! -f "$PKG_FILE" ]; then
    echo "❌ Aucun PKG trouvé"
    echo "Usage: $0 [chemin_vers_pkg]"
    exit 1
fi

echo "🔍 Vérification des binaires dans le PKG"
echo "PKG: $PKG_FILE"
echo ""

TEMP_DIR="/tmp/check_pkg_binaries_$$"
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

# Vérifier le type de PKG
PKG_TYPE=$(file "$PKG_FILE" 2>&1)
echo "Type de PKG: $PKG_TYPE"
echo ""

# Extraire le PKG (peut être xar ou bundle)
echo "📦 Extraction du PKG..."
if echo "$PKG_TYPE" | grep -q "xar archive"; then
    # Flat package (xar)
    xar -xf "$PKG_FILE" 2>&1 | head -3
elif [ -d "$PKG_FILE" ]; then
    # Bundle-style package (répertoire)
    echo "   Bundle-style package détecté, copie du contenu..."
    cp -R "$PKG_FILE/Contents" "$TEMP_DIR/" 2>&1 | head -3
    cd "$TEMP_DIR"
    if [ -d "Contents/Packages" ]; then
        # C'est un productbuild package avec sous-packages
        cd "Contents/Packages"
        for SUB_PKG in *.pkg; do
            if [ -d "$SUB_PKG" ]; then
                cp -R "$SUB_PKG" "$TEMP_DIR/" 2>&1
            fi
        done
        cd "$TEMP_DIR"
    fi
else
    echo "   ⚠️  Format de PKG non reconnu, tentative d'extraction xar..."
    xar -xf "$PKG_FILE" 2>&1 | head -3 || echo "   ❌ Échec de l'extraction"
fi
echo ""

# Vérifier les PackageInfo
echo "=== 1. Vérification des PackageInfo ==="
for PKG_INFO in *.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKG_NAME=$(basename $(dirname "$PKG_INFO"))
        echo "$PKG_NAME:"
        if grep -q 'hostArchitectures=' "$PKG_INFO"; then
            echo "  ✅ hostArchitectures: $(grep -o 'hostArchitectures="[^"]*"' "$PKG_INFO" | head -1)"
        else
            echo "  ❌ Pas d'attribut hostArchitectures"
        fi
    fi
done

echo ""
echo "=== 2. Vérification des binaires dans les packages ==="

# Extraire et vérifier les binaires de chaque sous-package
for PKG_DIR in *.pkg; do
    if [ -d "$PKG_DIR" ] && [ -f "$PKG_DIR/Payload" ]; then
        PKG_NAME=$(basename "$PKG_DIR")
        echo ""
        echo "$PKG_NAME:"
        
        PAYLOAD_DIR="$TEMP_DIR/payload_${PKG_NAME}"
        mkdir -p "$PAYLOAD_DIR"
        cd "$PAYLOAD_DIR"
        
        # Extraire le Payload
        cat "../$PKG_DIR/Payload" | gunzip | cpio -i 2>&1 | head -1 > /dev/null
        
        # Chercher tous les binaires exécutables
        BINARIES=$(find . -type f -perm +111 2>/dev/null | grep -E "\.app/Contents/MacOS/|\.component/Contents/MacOS/|\.vst3/Contents/MacOS/" | head -10)
        
        if [ -n "$BINARIES" ]; then
            for BIN in $BINARIES; do
                if [ -f "$BIN" ]; then
                    BIN_NAME=$(basename "$BIN")
                    echo "  $BIN_NAME:"
                    ARCH=$(lipo -info "$BIN" 2>&1)
                    if echo "$ARCH" | grep -q "arm64"; then
                        if echo "$ARCH" | grep -q "x86_64"; then
                            echo "    ✅ Universel: $ARCH"
                        else
                            echo "    ⚠️  ARM64 seulement: $ARCH"
                        fi
                    else
                        echo "    ❌ Pas ARM64: $ARCH"
                    fi
                fi
            done
        else
            echo "  ℹ️  Aucun binaire exécutable trouvé"
        fi
        
        cd "$TEMP_DIR"
        rm -rf "$PAYLOAD_DIR"
    fi
done

cd - > /dev/null
rm -rf "$TEMP_DIR"

echo ""
echo "💡 Si tous les binaires sont universels mais Rosetta est demandé,"
echo "   le problème peut venir de:"
echo "   1. La signature du PKG"
echo "   2. Un composant helper ou script"
echo "   3. Le format de hostArchitectures dans PackageInfo"

