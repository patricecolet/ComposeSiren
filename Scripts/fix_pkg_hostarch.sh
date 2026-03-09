#!/bin/bash
# Script pour corriger/ajouter hostArchitectures dans un PKG directement

PKG_FILE="$1"

if [ -z "$PKG_FILE" ]; then
    echo "❌ Usage: $0 <chemin_vers_pkg>"
    echo "   Exemple: $0 Releases/ComposeSiren_Orchestra-2.0.0-Darwin-universal.pkg"
    exit 1
fi

# Convertir en chemin absolu
if [ "${PKG_FILE#/}" = "$PKG_FILE" ]; then
    # Chemin relatif, le convertir en absolu
    PKG_FILE="$(cd "$(dirname "$PKG_FILE")" && pwd)/$(basename "$PKG_FILE")"
fi

if [ ! -f "$PKG_FILE" ]; then
    echo "❌ Fichier PKG non trouvé: $PKG_FILE"
    exit 1
fi

echo "🔧 Correction de hostArchitectures dans le PKG"
echo "PKG: $PKG_FILE"
echo ""

TEMP_DIR="/tmp/fix_pkg_hostarch_$$"
mkdir -p "$TEMP_DIR"
EXTRACT_DIR="$TEMP_DIR/extract"
mkdir -p "$EXTRACT_DIR"

# Extraire le PKG
echo "📦 Extraction du PKG..."
cd "$EXTRACT_DIR"
xar -xf "$PKG_FILE" 2>&1 | head -3

if [ ! -f "Distribution" ]; then
    echo "❌ Distribution.xml non trouvé après extraction"
    rm -rf "$TEMP_DIR"
    exit 1
fi

echo ""
echo "=== Correction des PackageInfo ==="
PKGINFO_FIXED=0
PKGINFO_ADDED=0

for PKG_INFO in *.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKG_NAME=$(basename $(dirname "$PKG_INFO"))
        
        # Vérifier si hostArchitectures existe
        if grep -q 'hostArchitectures=' "$PKG_INFO"; then
            # Existe déjà, vérifier le format
            PKGINFO_FORMAT=$(grep -o 'hostArchitectures="[^"]*"' "$PKG_INFO" 2>/dev/null | head -1)
            
            if ! echo "$PKGINFO_FORMAT" | grep -q 'hostArchitectures="x86_64 arm64"'; then
                echo "🔧 Correction du format dans $PKG_NAME/PackageInfo..."
                sed -i '' 's/hostArchitectures="x86_64,arm64"/hostArchitectures="x86_64 arm64"/g' "$PKG_INFO"
                sed -i '' 's/hostArchitectures="arm64,x86_64"/hostArchitectures="x86_64 arm64"/g' "$PKG_INFO"
                PKGINFO_FIXED=$((PKGINFO_FIXED + 1))
            else
                echo "✅ $PKG_NAME: Format déjà correct"
            fi
        else
            # N'existe pas, l'ajouter
            echo "➕ Ajout de hostArchitectures dans $PKG_NAME/PackageInfo..."
            # Ajouter après <pkg-info
            sed -i '' 's|<pkg-info\([^>]*\)>|<pkg-info\1 hostArchitectures="x86_64 arm64">|g' "$PKG_INFO"
            PKGINFO_ADDED=$((PKGINFO_ADDED + 1))
        fi
    fi
done

echo ""
if [ $PKGINFO_FIXED -gt 0 ] || [ $PKGINFO_ADDED -gt 0 ]; then
    if [ $PKGINFO_FIXED -gt 0 ]; then
        echo "✅ $PKGINFO_FIXED PackageInfo corrigé(s)"
    fi
    if [ $PKGINFO_ADDED -gt 0 ]; then
        echo "✅ $PKGINFO_ADDED PackageInfo avec hostArchitectures ajouté(s)"
    fi
    
    # Reconstruire le PKG
    echo ""
    echo "=== Reconstruction du PKG ==="
    OUTPUT_DIR="$(dirname "$PKG_FILE")"
    OUTPUT_NAME="$(basename "${PKG_FILE%.pkg}_fixed.pkg")"
    OUTPUT_PKG="$OUTPUT_DIR/$OUTPUT_NAME"
    
    # Essayer d'abord avec productbuild (plus fiable pour préserver les métadonnées)
    PKG_REBUILT=0
    if [ -f "Distribution" ]; then
        # Trouver les sous-packages
        PKG_PATHS=$(find . -maxdepth 1 -type d -name "*.pkg" | sed 's|^\./||' | tr '\n' ' ')
        if [ -n "$PKG_PATHS" ]; then
            # Créer le répertoire Resources s'il n'existe pas
            if [ ! -d "Resources" ]; then
                mkdir -p Resources
            fi
            echo "   🔨 Reconstruction avec productbuild..."
            if productbuild --distribution Distribution --package-path . --resources Resources "$OUTPUT_PKG" > /dev/null 2>&1; then
                echo "   ✅ PKG reconstruit avec productbuild"
                PKG_REBUILT=1
            else
                # Afficher l'erreur pour debug
                echo "   ⚠️  productbuild a échoué, erreur:"
                productbuild --distribution Distribution --package-path . --resources Resources "$OUTPUT_PKG" 2>&1 | grep -E "error|Error" | head -2 || true
            fi
        fi
    fi
    
    # Si productbuild a échoué, utiliser xar comme fallback
    if [ $PKG_REBUILT -eq 0 ]; then
        echo "   🔨 Reconstruction avec xar (fallback)..."
        xar -cf "$OUTPUT_PKG" * 2>&1 | head -3
        if [ -f "$OUTPUT_PKG" ] && [ -s "$OUTPUT_PKG" ]; then
            echo "   ✅ PKG reconstruit avec xar"
            PKG_REBUILT=1
        fi
    fi
    
    if [ $PKG_REBUILT -eq 1 ] && [ -f "$OUTPUT_PKG" ]; then
        echo "✅ PKG corrigé créé: $OUTPUT_PKG"
        echo ""
        echo "💡 Prochaine étape: Signer ce PKG avec productsign"
    else
        echo "❌ Échec de la reconstruction du PKG"
        rm -rf "$TEMP_DIR"
        exit 1
    fi
else
    echo "✅ Tous les PackageInfo sont déjà corrects"
    rm -rf "$TEMP_DIR"
    exit 0
fi

# Nettoyer
rm -rf "$TEMP_DIR"

