#!/bin/bash
# Script pour corriger les PackageInfo des sous-packages AVANT qu'ils ne soient assemblés par productbuild
# Ce script doit être appelé juste après que CPack ait créé les sous-packages mais avant l'assemblage final

PACKAGE_DIR="$1"
PROJECT_NAME="$2"

if [ -z "$PACKAGE_DIR" ] || [ ! -d "$PACKAGE_DIR" ]; then
    echo "❌ Répertoire non trouvé: $PACKAGE_DIR"
    exit 1
fi

if [ -z "$PROJECT_NAME" ]; then
    # Essayer de deviner depuis le répertoire
    # Le format est généralement BaseTargetName_Installer_artefacts
    DIR_NAME=$(basename "$PACKAGE_DIR")
    PROJECT_NAME=$(echo "$DIR_NAME" | sed 's/_Installer_artefacts$//' | sed 's/_artefacts$//')
    # Ajouter _Installer si ce n'est pas déjà présent
    if [[ ! "$PROJECT_NAME" =~ "_Installer$" ]]; then
        PROJECT_NAME="${PROJECT_NAME}_Installer"
    fi
fi

echo "🔧 Correction des PackageInfo des sous-packages..."
echo "Répertoire: $PACKAGE_DIR"
echo "Projet: $PROJECT_NAME"
echo ""

# Trouver tous les sous-packages créés par pkgbuild
# CPack crée les sous-packages dans _CPack_Packages/Darwin/productbuild/PACKAGE_NAME/Contents/Packages/
# Le nom du package CPack est différent du nom du projet (ex: ComposeSiren_Orchestra-2.0.0-Darwin-universal)
PRODUCTBUILD_BASE="$PACKAGE_DIR/_CPack_Packages/Darwin/productbuild"

# Trouver le répertoire productbuild (il n'y en a normalement qu'un)
PRODUCTBUILD_DIR=$(find "$PRODUCTBUILD_BASE" -type d -maxdepth 1 -mindepth 1 2>/dev/null | head -1)

if [ -z "$PRODUCTBUILD_DIR" ] || [ ! -d "$PRODUCTBUILD_DIR" ]; then
    echo "❌ Aucun répertoire productbuild trouvé dans $PRODUCTBUILD_BASE"
    exit 1
fi

SUBPACKAGES_DIR="$PRODUCTBUILD_DIR/Contents/Packages"

if [ ! -d "$SUBPACKAGES_DIR" ]; then
    echo "❌ Répertoire des sous-packages non trouvé: $SUBPACKAGES_DIR"
    exit 1
fi

echo "   Répertoire productbuild: $(basename "$PRODUCTBUILD_DIR")"

FIXED=0
MODIFIED_COUNT=0

# Parcourir tous les sous-packages (répertoires .pkg)
for PKG_DIR in "$SUBPACKAGES_DIR"/*.pkg; do
    if [ -d "$PKG_DIR" ]; then
        PKG_INFO="$PKG_DIR/PackageInfo"
        if [ -f "$PKG_INFO" ]; then
            PKG_NAME=$(basename "$PKG_DIR")
            MODIFIED=0
            
            # Vérifier si hostArchitectures existe déjà
            if grep -q 'hostArchitectures=' "$PKG_INFO"; then
                # Existe déjà, vérifier le format
                if ! grep -q 'hostArchitectures="x86_64 arm64"' "$PKG_INFO"; then
                    echo "   🔄 Correction du format dans $PKG_NAME..."
                    sed -i '' 's|hostArchitectures="[^"]*"|hostArchitectures="x86_64 arm64"|g' "$PKG_INFO"
                    MODIFIED=1
                fi
            else
                # N'existe pas, l'ajouter
                echo "   ➕ Ajout de hostArchitectures dans $PKG_NAME..."
                sed -i '' 's|<pkg-info\([^>]*\)>|<pkg-info\1 hostArchitectures="x86_64 arm64">|g' "$PKG_INFO"
                MODIFIED=1
            fi
            
            if [ $MODIFIED -eq 1 ]; then
                FIXED=1
                MODIFIED_COUNT=$((MODIFIED_COUNT + 1))
            fi
        fi
    fi
done

if [ $FIXED -eq 1 ]; then
    echo ""
    echo "✅ $MODIFIED_COUNT PackageInfo corrigé(s)"
else
    echo "✅ Tous les PackageInfo sont déjà corrects"
fi

exit 0

