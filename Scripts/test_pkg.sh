#!/bin/sh
# Script de test rapide pour vérifier qu'un PKG peut être ouvert

PKG_FILE="$1"

if [ -z "$PKG_FILE" ]; then
    echo "Usage: $0 <chemin_vers_pkg>"
    exit 1
fi

echo "🔍 Test du package: $PKG_FILE"
echo ""

# Test 1: Vérifier que le fichier existe
if [ ! -f "$PKG_FILE" ]; then
    echo "❌ Fichier non trouvé: $PKG_FILE"
    exit 1
fi

# Test 2: Vérifier le type de fichier
echo "📦 Type de fichier:"
file "$PKG_FILE"
echo ""

# Test 3: Vérifier la signature
echo "🔐 Signature:"
pkgutil --check-signature "$PKG_FILE" 2>&1
echo ""

# Test 4: Extraire et valider le Distribution.xml
TEMP_DIR="/tmp/test_pkg_$$"
mkdir -p "$TEMP_DIR"
echo "📄 Extraction et validation du Distribution.xml:"
if xar -xf "$PKG_FILE" -C "$TEMP_DIR" 2>&1; then
    if [ -f "$TEMP_DIR/Distribution" ]; then
        if command -v xmllint >/dev/null 2>&1; then
            if xmllint --noout "$TEMP_DIR/Distribution" 2>&1; then
                echo "   ✅ Distribution.xml valide"
                echo ""
                echo "   Contenu de hostArchitectures:"
                grep "hostArchitectures" "$TEMP_DIR/Distribution" | head -1
            else
                echo "   ❌ Distribution.xml invalide!"
                xmllint --noout "$TEMP_DIR/Distribution" 2>&1
            fi
        else
            echo "   ⚠️  xmllint non disponible, validation XML ignorée"
            grep "hostArchitectures" "$TEMP_DIR/Distribution" | head -1
        fi
    else
        echo "   ❌ Distribution.xml non trouvé"
    fi
else
    echo "   ❌ Impossible d'extraire le package"
fi

rm -rf "$TEMP_DIR"
echo ""
echo "✅ Test terminé"

