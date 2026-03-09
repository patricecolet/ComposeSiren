#!/bin/bash
# Script pour trouver le dernier PKG/DMG généré et afficher ses infos

echo "🔍 Recherche du dernier PKG/DMG généré..."
echo ""

# Chercher dans build/Packaging
BUILD_DIR="build/Packaging/ComposeSiren_Orchestra_Installer_artefacts"

if [ -d "$BUILD_DIR" ]; then
    # Chercher le PKG principal
    PKG_FILE=$(find "$BUILD_DIR" -maxdepth 1 -name "*.pkg" -type f -newer "$BUILD_DIR" 2>/dev/null | head -1)
    
    # Si pas trouvé, chercher dans dmgContent
    if [ -z "$PKG_FILE" ] && [ -d "$BUILD_DIR/dmgContent" ]; then
        PKG_FILE=$(find "$BUILD_DIR/dmgContent" -maxdepth 1 -name "*.pkg" -type f 2>/dev/null | head -1)
    fi
    
    # Chercher le DMG
    DMG_FILE=$(find "$BUILD_DIR" -maxdepth 1 -name "*.dmg" -type f 2>/dev/null | head -1)
    
    echo "📦 Répertoire de build: $BUILD_DIR"
    echo ""
    
    if [ -n "$PKG_FILE" ] && [ -f "$PKG_FILE" ]; then
        PKG_SIZE=$(ls -lh "$PKG_FILE" | awk '{print $5}')
        PKG_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" "$PKG_FILE" 2>/dev/null || stat -c "%y" "$PKG_FILE" 2>/dev/null | cut -d'.' -f1)
        echo "✅ PKG trouvé:"
        echo "   Chemin: $PKG_FILE"
        echo "   Taille: $PKG_SIZE"
        echo "   Date: $PKG_DATE"
        echo ""
        echo "💡 Pour vérifier les PackageInfo:"
        echo "   ./scripts/check_pkg_binaries.sh \"$PKG_FILE\""
    else
        echo "❌ Aucun PKG trouvé dans $BUILD_DIR"
    fi
    
    if [ -n "$DMG_FILE" ] && [ -f "$DMG_FILE" ]; then
        DMG_SIZE=$(ls -lh "$DMG_FILE" | awk '{print $5}')
        DMG_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" "$DMG_FILE" 2>/dev/null || stat -c "%y" "$DMG_FILE" 2>/dev/null | cut -d'.' -f1)
        echo "✅ DMG trouvé:"
        echo "   Chemin: $DMG_FILE"
        echo "   Taille: $DMG_SIZE"
        echo "   Date: $DMG_DATE"
        echo ""
        echo "💡 Pour tester dans la VM Sonoma:"
        echo "   Copie ce DMG dans la VM et double-clique dessus"
    else
        echo "❌ Aucun DMG trouvé dans $BUILD_DIR"
    fi
else
    echo "❌ Répertoire de build non trouvé: $BUILD_DIR"
    echo ""
    echo "💡 Cherche dans d'autres emplacements..."
    find build -name "*.dmg" -type f -mtime -1 2>/dev/null | head -3
fi



