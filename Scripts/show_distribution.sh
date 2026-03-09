#!/bin/bash
# Affiche le Distribution.xml d'un PKG

PKG_FILE="$1"

if [ -z "$PKG_FILE" ] || [ ! -f "$PKG_FILE" ]; then
    echo "❌ Usage: $0 <chemin_vers_pkg>"
    exit 1
fi

# Convertir en chemin absolu
if [ "${PKG_FILE#/}" = "$PKG_FILE" ]; then
    PKG_FILE="$(cd "$(dirname "$PKG_FILE")" && pwd)/$(basename "$PKG_FILE")"
fi

TEMP_DIR="/tmp/show_dist_$$"
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

echo "📦 Extraction du PKG: $PKG_FILE"
echo ""

xar -xf "$PKG_FILE" 2>&1 | head -3

if [ -f "Distribution" ]; then
    echo ""
    echo "=== Distribution.xml (premières 30 lignes) ==="
    head -30 "Distribution"
    echo ""
    echo "=== Recherche de hostArchitectures ==="
    grep -n "hostArchitectures" "Distribution" || echo "❌ hostArchitectures non trouvé"
    echo ""
    echo "=== Recherche de scripts (preinstall/postinstall) ==="
    find . -name "preinstall" -o -name "postinstall" -o -name "*.sh" | head -10
else
    echo "❌ Distribution.xml non trouvé"
fi

rm -rf "$TEMP_DIR"



