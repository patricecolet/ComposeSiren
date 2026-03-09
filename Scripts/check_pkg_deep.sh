#!/bin/bash
# Vérification approfondie d'un PKG pour comprendre pourquoi Rosetta est demandé

PKG_FILE="$1"

if [ -z "$PKG_FILE" ] || [ ! -f "$PKG_FILE" ]; then
    echo "❌ Usage: $0 <chemin_vers_pkg>"
    exit 1
fi

# Convertir en chemin absolu
if [ "${PKG_FILE#/}" = "$PKG_FILE" ]; then
    PKG_FILE="$(cd "$(dirname "$PKG_FILE")" && pwd)/$(basename "$PKG_FILE")"
fi

TEMP_DIR="/tmp/check_pkg_deep_$$"
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

echo "🔍 Analyse approfondie du PKG"
echo "PKG: $PKG_FILE"
echo ""

xar -xf "$PKG_FILE" 2>&1 | head -3

echo ""
echo "=== 1. Distribution.xml ==="
if [ -f "Distribution" ]; then
    grep -n "hostArchitectures" "Distribution" || echo "❌ hostArchitectures non trouvé"
else
    echo "❌ Distribution.xml non trouvé"
fi

echo ""
echo "=== 2. PackageInfo de chaque sous-package ==="
for PKG_DIR in *.pkg; do
    if [ -d "$PKG_DIR" ]; then
        PKG_NAME=$(basename "$PKG_DIR")
        echo ""
        echo "📦 $PKG_NAME:"
        
        if [ -f "$PKG_DIR/PackageInfo" ]; then
            echo "   PackageInfo hostArchitectures:"
            grep -o 'hostArchitectures="[^"]*"' "$PKG_DIR/PackageInfo" | head -1 || echo "   ❌ Non trouvé"
        fi
        
        # Vérifier les scripts
        if [ -f "$PKG_DIR/Scripts/preinstall" ] || [ -f "$PKG_DIR/Scripts/postinstall" ]; then
            echo "   Scripts trouvés:"
            [ -f "$PKG_DIR/Scripts/preinstall" ] && echo "   - preinstall"
            [ -f "$PKG_DIR/Scripts/postinstall" ] && echo "   - postinstall"
            
            # Vérifier l'architecture des scripts
            for SCRIPT in "$PKG_DIR/Scripts"/*; do
                if [ -f "$SCRIPT" ] && [ -x "$SCRIPT" ]; then
                    SCRIPT_ARCH=$(file "$SCRIPT" 2>&1)
                    echo "     $(basename "$SCRIPT"): $SCRIPT_ARCH"
                fi
            done
        fi
        
        # Extraire et vérifier les binaires dans le Payload
        if [ -f "$PKG_DIR/Payload" ]; then
            PAYLOAD_DIR="$TEMP_DIR/payload_${PKG_NAME}"
            mkdir -p "$PAYLOAD_DIR"
            cd "$PAYLOAD_DIR"
            
            cat "../$PKG_DIR/Payload" | gunzip | cpio -i 2>&1 | head -1 > /dev/null
            
            # Chercher les binaires principaux
            BINARIES=$(find . -type f -perm +111 2>/dev/null | grep -E "\.app/Contents/MacOS/|\.component/Contents/MacOS/|\.vst3/Contents/MacOS/" | head -3)
            
            if [ -n "$BINARIES" ]; then
                echo "   Binaires dans Payload:"
                for BIN in $BINARIES; do
                    if [ -f "$BIN" ]; then
                        ARCH=$(lipo -info "$BIN" 2>&1)
                        echo "     $(basename "$BIN"): $ARCH"
                    fi
                done
            fi
            
            cd "$TEMP_DIR"
            rm -rf "$PAYLOAD_DIR"
        fi
    fi
done

rm -rf "$TEMP_DIR"



