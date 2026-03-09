#!/bin/bash
# Script pour reconstruire le PKG principal avec les PackageInfo corrigés
# Ce script doit être appelé après fix_pkginfo_before_productbuild.sh

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

echo "🔨 Reconstruction du PKG principal avec les PackageInfo corrigés..."
echo "Répertoire: $PACKAGE_DIR"
echo "Projet: $PROJECT_NAME"
echo ""

# Trouver le répertoire productbuild
# CPack utilise le nom du package (ex: ComposeSiren_Orchestra-2.0.0-Darwin-universal), pas le nom du projet
PRODUCTBUILD_BASE="$PACKAGE_DIR/_CPack_Packages/Darwin/productbuild"

# Trouver le répertoire productbuild (il n'y en a normalement qu'un)
PRODUCTBUILD_DIR=$(find "$PRODUCTBUILD_BASE" -type d -maxdepth 1 -mindepth 1 2>/dev/null | head -1)

if [ -z "$PRODUCTBUILD_DIR" ] || [ ! -d "$PRODUCTBUILD_DIR" ]; then
    echo "❌ Aucun répertoire productbuild trouvé dans $PRODUCTBUILD_BASE"
    exit 1
fi

echo "   Répertoire productbuild: $(basename "$PRODUCTBUILD_DIR")"

cd "$PRODUCTBUILD_DIR"

# Vérifier que distribution.dist existe (CPack crée distribution.dist, pas Distribution)
if [ ! -f "Contents/distribution.dist" ]; then
    echo "❌ distribution.dist non trouvé dans $PRODUCTBUILD_DIR/Contents/"
    exit 1
fi

# Vérifier que les Packages existent
if [ ! -d "Contents/Packages" ]; then
    echo "❌ Répertoire Contents/Packages non trouvé"
    exit 1
fi

# Déterminer le nom du PKG final
# Le nom du PKG correspond au nom du répertoire productbuild + .pkg
PACKAGE_NAME=$(basename "$PRODUCTBUILD_DIR")
PKG_NAME="${PACKAGE_NAME}.pkg"

# Chercher le PKG existant dans le répertoire parent ou dans les artefacts
if [ -f "../$PKG_NAME" ]; then
    # Le PKG existe déjà dans le répertoire parent
    EXISTING_PKG="../$PKG_NAME"
elif [ -f "$PACKAGE_DIR/$PKG_NAME" ]; then
    # Le PKG existe dans le répertoire artefacts
    EXISTING_PKG="$PACKAGE_DIR/$PKG_NAME"
else
    # Utiliser le nom trouvé
    EXISTING_PKG=""
fi

echo "   📦 Reconstruction de $PKG_NAME..."

# Créer le répertoire Resources s'il n'existe pas
if [ ! -d "Contents/Resources" ]; then
    mkdir -p "Contents/Resources"
fi

# Reconstruire le PKG avec productbuild en utilisant distribution.dist
TEMP_PKG="../${PKG_NAME}.tmp"
PRODUCTBUILD_OUTPUT=$(productbuild --distribution Contents/distribution.dist --package-path Contents/Packages --resources Contents/Resources "$TEMP_PKG" 2>&1)
PRODUCTBUILD_EXIT_CODE=$?

if [ $PRODUCTBUILD_EXIT_CODE -eq 0 ] && [ -f "$TEMP_PKG" ] && [ -s "$TEMP_PKG" ]; then
    # Succès, remplacer l'original
    if [ -n "$EXISTING_PKG" ] && [ -f "$EXISTING_PKG" ]; then
        mv "$EXISTING_PKG" "${EXISTING_PKG}.backup"
    fi
    
    # Déplacer le PKG temporaire vers le répertoire artefacts principal
    mv "$TEMP_PKG" "$PACKAGE_DIR/$PKG_NAME"
    
    # Copier aussi dans le répertoire parent productbuild si nécessaire
    if [ -f "$PACKAGE_DIR/$PKG_NAME" ]; then
        cp "$PACKAGE_DIR/$PKG_NAME" "../$PKG_NAME" 2>/dev/null || true
    fi
    
    echo "   ✅ PKG reconstruit avec succès: $PACKAGE_DIR/$PKG_NAME"
    
    # Nettoyer le backup
    if [ -n "$EXISTING_PKG" ] && [ -f "${EXISTING_PKG}.backup" ]; then
        rm -f "${EXISTING_PKG}.backup"
    fi
else
    echo "   ❌ Échec de la reconstruction avec productbuild (code: $PRODUCTBUILD_EXIT_CODE):"
    echo "$PRODUCTBUILD_OUTPUT" | grep -E "error|Error|warning|Warning" | head -10 || echo "$PRODUCTBUILD_OUTPUT" | tail -5
    rm -f "$TEMP_PKG"
    exit 1
fi

cd - > /dev/null
exit 0

