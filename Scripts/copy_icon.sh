#!/bin/bash
# Script pour copier l'icône dans l'application bundle
# À utiliser comme Build Phase dans Xcode

set -e

ICON_SOURCE="${PROJECT_DIR}/Assets/ComposeSiren.icns"
ICON_DEST="${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources/ComposeSiren.icns"

# Vérifier que l'icône source existe
if [ ! -f "$ICON_SOURCE" ]; then
    echo "⚠️  Icône source introuvable: $ICON_SOURCE"
    exit 1
fi

# Créer le dossier Resources s'il n'existe pas
mkdir -p "${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources"

# Copier l'icône
echo "📦 Copie de l'icône: $ICON_SOURCE → $ICON_DEST"
cp "$ICON_SOURCE" "$ICON_DEST"

# Rafraîchir l'enregistrement de l'app
echo "🔄 Rafraîchissement de l'enregistrement macOS..."
touch "${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app"

echo "✅ Icône copiée avec succès!"

