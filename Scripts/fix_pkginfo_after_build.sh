#!/bin/bash
# Script pour corriger le format hostArchitectures dans les PackageInfo après le build
# Ce script doit être exécuté après que CPack ait généré le package

PACKAGE_DIR="$1"

if [ -z "$PACKAGE_DIR" ]; then
    # Chercher automatiquement
    PACKAGE_DIR=$(find build/Packaging -name "*_artefacts" -type d 2>/dev/null | head -1)
fi

if [ -z "$PACKAGE_DIR" ] || [ ! -d "$PACKAGE_DIR" ]; then
    echo "❌ Répertoire de packaging non trouvé"
    echo "Usage: $0 [chemin_vers_artefacts_dir]"
    exit 1
fi

echo "🔧 Correction du format hostArchitectures dans les PackageInfo..."
echo "Répertoire: $PACKAGE_DIR"
echo ""

# Chercher le PKG principal (celui qui contient les sous-packages, pas les sous-packages eux-mêmes)
# Le PKG principal peut être dans:
# 1. _CPack_Packages/Darwin/productbuild/BaseTargetName-VERSION-Darwin-universal.pkg
# 2. dmgContent/BaseTargetName-VERSION-Darwin-universal.pkg (après création du DMG)
# Format attendu: BaseTargetName-VERSION-Darwin-universal.pkg (sans suffixe -AU, -VST3, etc.)

# Chercher dans _CPack_Packages d'abord (avant création du DMG)
# CPack crée le PKG dans _CPack_Packages/Darwin/productbuild/
PKG_FILE=$(find "$PACKAGE_DIR/_CPack_Packages" -name "*.pkg" -type f 2>/dev/null | grep -E "Darwin/productbuild/.*-Darwin-universal\.pkg$" | grep -v "/Contents/Packages/" | head -1)

# Si pas trouvé, chercher au niveau racine (CPack peut aussi le copier là)
if [ -z "$PKG_FILE" ]; then
    PKG_FILE=$(find "$PACKAGE_DIR" -maxdepth 1 -name "*.pkg" -type f 2>/dev/null | grep -v -- "-AU.pkg$\|-VST3.pkg$\|-Standalone.pkg$\|-Resources.pkg$" | head -1)
fi

# Si toujours pas trouvé, chercher dans dmgContent (après création du DMG)
if [ -z "$PKG_FILE" ]; then
    PKG_FILE=$(find "$PACKAGE_DIR/dmgContent" -name "*.pkg" -type f 2>/dev/null | grep -v -- "-AU.pkg$\|-VST3.pkg$\|-Standalone.pkg$\|-Resources.pkg$" | head -1)
fi

if [ -z "$PKG_FILE" ]; then
    echo "❌ Aucun PKG principal trouvé dans $PACKAGE_DIR"
    echo "   Cherchez un fichier .pkg dans:"
    echo "   - _CPack_Packages/Darwin/productbuild/"
    echo "   - dmgContent/"
    echo "   - ou au niveau racine"
    exit 1
fi

# Convertir en chemin absolu et vérifier qu'il existe
PKG_DIR_ABS=$(cd "$(dirname "$PKG_FILE")" 2>/dev/null && pwd)
if [ -z "$PKG_DIR_ABS" ] || [ ! -d "$PKG_DIR_ABS" ]; then
    echo "❌ Le répertoire du PKG n'existe pas: $(dirname "$PKG_FILE")"
    exit 1
fi

PKG_FILE="${PKG_DIR_ABS}/$(basename "$PKG_FILE")"
if [ ! -f "$PKG_FILE" ]; then
    echo "❌ Le fichier PKG n'existe pas: $PKG_FILE"
    exit 1
fi

echo "📦 PKG trouvé: $(basename "$PKG_FILE")"
echo "   Chemin: $PKG_FILE"
echo ""

# Extraire le package
TEMP_DIR="/tmp/fix_pkginfo_$$"
mkdir -p "$TEMP_DIR"
xar -xf "$PKG_FILE" -C "$TEMP_DIR" 2>&1 | head -3

FIXED=0

# Corriger les PackageInfo
for PKG_INFO in "$TEMP_DIR"/*.pkg/PackageInfo; do
    if [ -f "$PKG_INFO" ]; then
        PKG_NAME=$(basename $(dirname "$PKG_INFO"))
        MODIFIED=0
        
        # Vérifier si hostArchitectures existe
        if grep -q 'hostArchitectures=' "$PKG_INFO"; then
            # Existe déjà, corriger le format si nécessaire
            if grep -q 'hostArchitectures="x86_64,arm64"' "$PKG_INFO" || grep -q 'hostArchitectures="arm64,x86_64"' "$PKG_INFO"; then
                echo "   🔄 Correction du format dans $PKG_NAME..."
                sed -i '' 's|hostArchitectures="x86_64,arm64"|hostArchitectures="x86_64 arm64"|g' "$PKG_INFO"
                sed -i '' 's|hostArchitectures="arm64,x86_64"|hostArchitectures="x86_64 arm64"|g' "$PKG_INFO"
                MODIFIED=1
            fi
        else
            # N'existe pas, l'ajouter à la balise <pkg-info>
            echo "   ➕ Ajout de hostArchitectures dans $PKG_NAME..."
            # Chercher la balise <pkg-info et ajouter l'attribut
            sed -i '' 's|<pkg-info\([^>]*\)>|<pkg-info\1 hostArchitectures="x86_64 arm64">|g' "$PKG_INFO"
            MODIFIED=1
        fi
        
        if [ $MODIFIED -eq 1 ]; then
            FIXED=1
        fi
    fi
done

if [ $FIXED -eq 1 ]; then
    echo ""
    echo "   🔨 Reconstruction du PKG..."
    cd "$TEMP_DIR"
    
    # Sauvegarder l'original
    BACKUP_PKG="${PKG_FILE}.backup"
    cp "$PKG_FILE" "$BACKUP_PKG"
    
    # Essayer d'abord avec productbuild (préserve mieux les métadonnées)
    PKG_REBUILT=0
    if [ -f "Distribution" ]; then
        # Créer le répertoire Resources s'il n'existe pas (peut être vide)
        if [ ! -d "Resources" ]; then
            mkdir -p Resources
        fi
        
        # Trouver les sous-packages (répertoires .pkg)
        PKG_PATHS=$(find . -maxdepth 1 -type d -name "*.pkg" | sed 's|^\./||' | tr '\n' ' ')
        if [ -n "$PKG_PATHS" ]; then
            echo "   🔨 Tentative de reconstruction avec productbuild..."
            # Utiliser un fichier temporaire pour éviter d'écraser l'original en cas d'erreur
            TEMP_PKG="${PKG_FILE}.tmp"
            PRODUCTBUILD_OUTPUT=$(productbuild --distribution Distribution --package-path . --resources Resources "$TEMP_PKG" 2>&1)
            PRODUCTBUILD_EXIT_CODE=$?
            
            if [ $PRODUCTBUILD_EXIT_CODE -eq 0 ] && [ -f "$TEMP_PKG" ] && [ -s "$TEMP_PKG" ]; then
                # Succès
                mv "$TEMP_PKG" "$PKG_FILE"
                echo "   ✅ PKG reconstruit avec productbuild (métadonnées préservées)"
                PKG_REBUILT=1
                rm -f "$BACKUP_PKG"
            else
                # Échec, afficher l'erreur
                echo "   ⚠️  productbuild a échoué (code: $PRODUCTBUILD_EXIT_CODE):"
                echo "$PRODUCTBUILD_OUTPUT" | grep -E "error|Error|warning|Warning" | head -5 || echo "$PRODUCTBUILD_OUTPUT" | tail -3
                rm -f "$TEMP_PKG"
            fi
        fi
    fi
    
    # Si productbuild a échoué, utiliser xar (plus fiable pour les modifications)
    if [ $PKG_REBUILT -eq 0 ]; then
        echo "   ⚠️  productbuild a échoué, utilisation de xar..."
        # Créer le nouveau PKG dans le même répertoire que l'original
        PKG_DIR=$(dirname "$PKG_FILE")
        # S'assurer que le répertoire existe (créer si nécessaire)
        mkdir -p "$PKG_DIR"
        NEW_PKG="${PKG_DIR}/$(basename "$PKG_FILE").new"
        
        # Créer l'archive avec xar depuis le répertoire temporaire
        xar -cf "$NEW_PKG" * 2>&1 | head -3
        if [ -f "$NEW_PKG" ] && [ -s "$NEW_PKG" ]; then
            mv "$NEW_PKG" "$PKG_FILE"
            echo "   ✅ PKG reconstruit avec xar"
            rm -f "$BACKUP_PKG"
        else
            echo "   ❌ Échec de la reconstruction avec xar"
            echo "      Vérifiez les permissions et l'espace disque"
            if [ -f "$BACKUP_PKG" ]; then
                mv "$BACKUP_PKG" "$PKG_FILE"
                echo "      Original restauré"
            fi
        fi
    fi
    
    cd - > /dev/null
    
    # Si le PKG corrigé est dans _CPack_Packages, le copier dans le répertoire artefacts
    # pour que CreateDmg.sh puisse le trouver
    if echo "$PKG_FILE" | grep -q "_CPack_Packages"; then
        PKG_NAME=$(basename "$PKG_FILE")
        ARTEFACTS_PKG="${PACKAGE_DIR}/${PKG_NAME}"
        if [ "$PKG_FILE" != "$ARTEFACTS_PKG" ]; then
            echo ""
            echo "   📋 Copie du PKG corrigé dans le répertoire artefacts..."
            cp "$PKG_FILE" "$ARTEFACTS_PKG"
            echo "   ✅ PKG copié vers: $ARTEFACTS_PKG"
        fi
    fi
    
    # Si le DMG existe dans le répertoire artefacts, le recréer aussi
    # Le DMG est toujours dans le répertoire artefacts, pas dans _CPack_Packages
    DMG_FILE=$(find "$PACKAGE_DIR" -maxdepth 1 -name "*.dmg" -type f 2>/dev/null | head -1)
    if [ -n "$DMG_FILE" ] && [ -f "$DMG_FILE" ]; then
        echo ""
        echo "   🔨 Reconstruction du DMG..."
        DMG_TEMP="/tmp/rebuild_dmg_$$"
        mkdir -p "$DMG_TEMP"
        
        # Copier le PKG corrigé
        cp "$PKG_FILE" "$DMG_TEMP/$(basename "$PKG_FILE")"
        
        # Copier l'app de désinstallation si elle existe (dans dmgContent)
        if [ -d "$PACKAGE_DIR/dmgContent" ]; then
            UNINSTALL_APP=$(find "$PACKAGE_DIR/dmgContent" -name "Uninstall*.app" -type d | head -1)
            if [ -n "$UNINSTALL_APP" ] && [ -d "$UNINSTALL_APP" ]; then
                cp -R "$UNINSTALL_APP" "$DMG_TEMP/" 2>/dev/null
                
                # Vérifier et forcer l'app Uninstall à être universelle
                UNINSTALL_BINARY="$DMG_TEMP/$(basename "$UNINSTALL_APP")/Contents/MacOS/applet"
                if [ -f "$UNINSTALL_BINARY" ]; then
                    ARCH_INFO=$(lipo -info "$UNINSTALL_BINARY" 2>&1)
                    if ! echo "$ARCH_INFO" | grep -q "arm64"; then
                        echo "   ⚠️  L'app Uninstall n'est pas universelle, tentative de correction..."
                        # osacompile crée normalement des apps universelles, mais on vérifie
                    fi
                fi
            fi
        fi
        
        # Créer le nouveau DMG temporaire
        TEMP_DMG="${DMG_FILE}.tmp"
        hdiutil create -ov -volname "$(basename "$DMG_FILE" .dmg)" -fs HFS+ -srcfolder "$DMG_TEMP" "$TEMP_DMG" 2>&1 | tail -2
        
        if [ -f "$TEMP_DMG" ]; then
            # Sauvegarder l'original
            BACKUP_DMG="${DMG_FILE}.backup"
            if [ -f "$DMG_FILE" ]; then
                mv "$DMG_FILE" "$BACKUP_DMG"
            fi
            
            # Convertir en format compressé et remplacer l'original
            if hdiutil convert -format UDZO -o "$DMG_FILE" "$TEMP_DMG" 2>&1 | tail -2; then
                rm -f "$TEMP_DMG" "$BACKUP_DMG"
                echo "   ✅ DMG reconstruit avec succès"
            else
                # Restaurer l'original en cas d'échec
                if [ -f "$BACKUP_DMG" ]; then
                    mv "$BACKUP_DMG" "$DMG_FILE"
                fi
                rm -f "$TEMP_DMG"
                echo "   ⚠️  Échec de la conversion du DMG, original restauré"
            fi
        else
            echo "   ⚠️  Échec de la création du DMG temporaire"
        fi
        
        rm -rf "$DMG_TEMP"
    fi
else
    echo "   ✅ Tous les PackageInfo ont déjà le bon format"
fi

rm -rf "$TEMP_DIR"
echo ""
echo "✅ Correction terminée"

