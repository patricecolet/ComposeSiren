#!/bin/sh
# Script pour modifier le Distribution.xml dans un PKG existant
# Usage: ./fix_pkg.sh <chemin_vers_pkg>

PKG_FILE="$1"
TEMP_DIR="/tmp/fix_pkg_$$"

if [ -z "$PKG_FILE" ]; then
    echo "Usage: $0 <chemin_vers_pkg>"
    echo "Exemple: $0 Releases/v2.0.1-macOS/ComposeSiren-v2.0.1-macOS.dmg"
    exit 1
fi

# Convertir en chemin absolu si nécessaire
if [ ! -f "$PKG_FILE" ] && [ -f "$(pwd)/$PKG_FILE" ]; then
    PKG_FILE="$(pwd)/$PKG_FILE"
fi

# Convertir en chemin absolu
if [ "$(echo "$PKG_FILE" | cut -c1)" != "/" ]; then
    PKG_FILE="$(cd "$(dirname "$PKG_FILE")" && pwd)/$(basename "$PKG_FILE")"
fi

# Si c'est un DMG, extraire le PKG d'abord
ORIGINAL_DMG=""
if echo "$PKG_FILE" | grep -q "\.dmg$"; then
    ORIGINAL_DMG="$PKG_FILE"
    MOUNT_POINT="/tmp/fix_dmg_$$"
    # Démontage forcé de toute instance existante
    hdiutil detach "$MOUNT_POINT" -force 2>/dev/null
    # Démontage de toute instance montée du même DMG
    hdiutil info | grep -A 5 "$PKG_FILE" | grep "/dev/disk" | awk '{print $1}' | xargs -I {} hdiutil detach {} -force 2>/dev/null
    sleep 1
    hdiutil attach -quiet "$PKG_FILE" -mountpoint "$MOUNT_POINT" -nobrowse 2>/dev/null || {
        echo "❌ Impossible de monter le DMG"
        exit 1
    }
    PKG_IN_DMG=$(find "$MOUNT_POINT" -name "*.pkg" | head -1)
    if [ -z "$PKG_IN_DMG" ]; then
        echo "❌ Aucun PKG trouvé dans le DMG"
        hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
        exit 1
    fi
    ORIGINAL_PKG_NAME=$(basename "$PKG_IN_DMG")
    echo "📦 PKG trouvé dans DMG: $ORIGINAL_PKG_NAME"
    # Copier le PKG vers un emplacement modifiable
    PKG_FILE="/tmp/fix_pkg_work_$$.pkg"
    cp "$PKG_IN_DMG" "$PKG_FILE"
    hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
    IS_DMG=1
else
    IS_DMG=0
fi

if [ ! -f "$PKG_FILE" ]; then
    echo "❌ PKG non trouvé: $PKG_FILE"
    [ "$IS_DMG" = "1" ] && hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
    exit 1
fi

echo "🔧 Modification du PKG: $PKG_FILE"

# Extraire avec xar
cd /tmp && rm -rf "$TEMP_DIR" && mkdir -p "$TEMP_DIR"
xar -xf "$PKG_FILE" -C "$TEMP_DIR" || {
    echo "❌ Erreur lors de l'extraction"
    [ "$IS_DMG" = "1" ] && hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
    rm -rf "$TEMP_DIR"
    exit 1
}

# Modifier Distribution.xml
if [ -f "$TEMP_DIR/Distribution" ]; then
    echo "📄 Distribution.xml trouvé"
    
    # Vérifier et corriger hostArchitectures dans installer-gui-script
    if grep -q "hostArchitectures" "$TEMP_DIR/Distribution"; then
        echo "   ℹ️  hostArchitectures déjà présent dans installer-gui-script:"
        grep "hostArchitectures" "$TEMP_DIR/Distribution" | head -1
        # Convertir les virgules en espaces si nécessaire
        if grep -q 'hostArchitectures="x86_64,arm64"' "$TEMP_DIR/Distribution"; then
            echo "   🔄 Conversion des virgules en espaces..."
            sed -i '' 's|hostArchitectures="x86_64,arm64"|hostArchitectures="x86_64 arm64"|g' "$TEMP_DIR/Distribution"
            echo "   ✅ Format corrigé (espaces au lieu de virgules)"
        fi
    else
        echo "   ➕ Ajout de hostArchitectures=\"x86_64 arm64\" à installer-gui-script..."
        sed -i '' 's|<installer-gui-script minSpecVersion="1">|<installer-gui-script minSpecVersion="1" hostArchitectures="x86_64 arm64">|' "$TEMP_DIR/Distribution"
        
        if grep -q "hostArchitectures" "$TEMP_DIR/Distribution"; then
            echo "   ✅ hostArchitectures ajouté avec succès"
        else
            echo "   ❌ Échec de l'ajout"
            [ "$IS_DMG" = "1" ] && hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
            rm -rf "$TEMP_DIR"
            exit 1
        fi
    fi
    
    # Réparer la balise <options> corrompue si nécessaire
    if grep -q '/> hostArchitectures=' "$TEMP_DIR/Distribution"; then
        echo "   🔧 Réparation de la balise <options> corrompue..."
        sed -i '' 's|/> hostArchitectures="[^"]*">|/>|' "$TEMP_DIR/Distribution"
        echo "   ✅ Balise <options> réparée"
    fi
    # Vérifier et corriger la balise <options> pour qu'elle soit valide
    OPTIONS_LINE=$(grep '<options' "$TEMP_DIR/Distribution" || echo "")
    if [ -n "$OPTIONS_LINE" ]; then
        # Si la ligne contient des caractères après />, c'est corrompu
        if echo "$OPTIONS_LINE" | grep -q '/> [^<]'; then
            echo "   🔧 Correction de la balise <options> corrompue..."
            # Extraire les attributs valides avant />
            OPTIONS_ATTRS=$(echo "$OPTIONS_LINE" | sed 's|.*<options\([^>]*\)/>.*|\1|' | sed 's|hostArchitectures="[^"]*"||g' | sed 's|  *| |g')
            # Remplacer toute la ligne par une version propre
            sed -i '' "s|.*<options.*|    <options${OPTIONS_ATTRS}/>|" "$TEMP_DIR/Distribution"
            echo "   ✅ Balise <options> corrigée"
        fi
        # S'assurer que la balise se termine bien par />
        if ! echo "$OPTIONS_LINE" | grep -q '/>$'; then
            echo "   🔧 Correction du format de la balise <options>..."
            sed -i '' 's|<options\([^>]*\)>\([^<]*\)</options>|<options\1/>|' "$TEMP_DIR/Distribution"
            sed -i '' 's|<options\([^>]*\)>\([^<]*\)|<options\1/>|' "$TEMP_DIR/Distribution"
        fi
    fi
    
    # Valider le XML avec xmllint si disponible
    if command -v xmllint >/dev/null 2>&1; then
        if xmllint --noout "$TEMP_DIR/Distribution" 2>/dev/null; then
            echo "   ✅ Distribution.xml valide"
        else
            echo "   ⚠️  Distribution.xml invalide, tentative de correction..."
            xmllint --format "$TEMP_DIR/Distribution" > "$TEMP_DIR/Distribution.fixed" 2>/dev/null
            if [ -f "$TEMP_DIR/Distribution.fixed" ]; then
                mv "$TEMP_DIR/Distribution.fixed" "$TEMP_DIR/Distribution"
                echo "   ✅ Distribution.xml corrigé"
            fi
        fi
    fi
    
    # Note: hostArchitectures dans <options> n'est pas nécessaire selon la doc Apple
    # Il suffit de l'avoir dans <installer-gui-script> et dans les PackageInfo
    
    # Vérifier et modifier les packages individuels si nécessaire
    echo "   🔍 Vérification des packages individuels..."
    for INDIVIDUAL_PKG in "$TEMP_DIR"/*.pkg; do
        if [ -d "$INDIVIDUAL_PKG" ]; then
            PKG_NAME=$(basename "$INDIVIDUAL_PKG")
            echo "      Vérification de $PKG_NAME..."
            # Vérifier PackageInfo dans les packages individuels
            if [ -f "$INDIVIDUAL_PKG/PackageInfo" ]; then
                if ! grep -q "hostArchitectures" "$INDIVIDUAL_PKG/PackageInfo"; then
                    echo "         ➕ Ajout de hostArchitectures au PackageInfo..."
                    # Ajouter hostArchitectures dans la balise <pkg-info>
                    sed -i '' 's|<pkg-info\([^>]*\)>|<pkg-info\1 hostArchitectures="x86_64 arm64">|' "$INDIVIDUAL_PKG/PackageInfo"
                    if grep -q "hostArchitectures" "$INDIVIDUAL_PKG/PackageInfo"; then
                        echo "         ✅ hostArchitectures ajouté au PackageInfo"
                    fi
                else
                    echo "         ✅ hostArchitectures déjà présent dans PackageInfo"
                    # Convertir les virgules en espaces si nécessaire
                    if grep -q 'hostArchitectures="x86_64,arm64"' "$INDIVIDUAL_PKG/PackageInfo"; then
                        echo "         🔄 Conversion des virgules en espaces dans PackageInfo..."
                        sed -i '' 's|hostArchitectures="x86_64,arm64"|hostArchitectures="x86_64 arm64"|g' "$INDIVIDUAL_PKG/PackageInfo"
                        echo "         ✅ Format corrigé (espaces au lieu de virgules)"
                    fi
                fi
            fi
        fi
    done
    
    # Recréer le PKG avec productbuild pour s'assurer que tout est correct
    echo "   🔨 Reconstruction du PKG avec productbuild..."
    cd "$TEMP_DIR"
    
    # Vérifier que les modifications ont bien été appliquées
    echo "   🔍 Vérification finale des modifications..."
    if ! grep -q 'hostArchitectures="x86_64 arm64"' "$TEMP_DIR/Distribution"; then
        echo "   ⚠️  Attention: hostArchitectures non trouvé dans Distribution après modifications"
    else
        echo "   ✅ hostArchitectures confirmé dans Distribution"
    fi
    
    # Utiliser productbuild au lieu de xar pour mieux gérer les architectures
    if [ -f "Distribution" ] && [ -d "Resources" ]; then
        # Trouver les packages individuels
        PKG_PATHS=$(find . -maxdepth 1 -type d -name "*.pkg" | sed 's|^\./||' | tr '\n' ' ')
        if [ -n "$PKG_PATHS" ]; then
            # Utiliser productbuild avec --timestamp pour préserver les métadonnées
            # Valider le Distribution.xml avant de reconstruire
            if command -v xmllint >/dev/null 2>&1; then
                if ! xmllint --noout Distribution 2>/dev/null; then
                    echo "   ❌ Distribution.xml invalide, impossible de reconstruire avec productbuild"
                    echo "   ⚠️  Utilisation de xar en dernier recours..."
                    xar -cf "$PKG_FILE" * || {
                        echo "   ❌ Erreur lors de la reconstruction"
                        rm -rf "$TEMP_DIR"
                        [ "$IS_DMG" = "1" ] && rm -f "$PKG_FILE"
                        exit 1
                    }
                else
                    productbuild --distribution Distribution --package-path . --resources Resources --timestamp "$PKG_FILE" 2>&1 || {
                        echo "   ⚠️  productbuild avec --timestamp a échoué, tentative sans..."
                        productbuild --distribution Distribution --package-path . --resources Resources "$PKG_FILE" 2>&1 || {
                            echo "   ❌ productbuild a échoué. Le Distribution.xml peut être invalide."
                            echo "   Vérifiez le fichier Distribution dans $TEMP_DIR"
                            rm -rf "$TEMP_DIR"
                            [ "$IS_DMG" = "1" ] && rm -f "$PKG_FILE"
                            exit 1
                        }
                    }
                fi
            else
                # xmllint non disponible, essayer productbuild directement
                productbuild --distribution Distribution --package-path . --resources Resources "$PKG_FILE" 2>&1 || {
                    echo "   ❌ productbuild a échoué"
                    echo "   Vérifiez le fichier Distribution dans $TEMP_DIR"
                    rm -rf "$TEMP_DIR"
                    [ "$IS_DMG" = "1" ] && rm -f "$PKG_FILE"
                    exit 1
                }
            fi
        else
            echo "   ⚠️  Aucun package individuel trouvé, utilisation de xar..."
            xar -cf "$PKG_FILE" * || {
                echo "   ❌ Erreur lors de la reconstruction"
                rm -rf "$TEMP_DIR"
                [ "$IS_DMG" = "1" ] && rm -f "$PKG_FILE"
                exit 1
            }
        fi
    else
        echo "   ⚠️  Structure inattendue, utilisation de xar..."
        xar -cf "$PKG_FILE" * || {
            echo "   ❌ Erreur lors de la reconstruction"
            rm -rf "$TEMP_DIR"
            [ "$IS_DMG" = "1" ] && rm -f "$PKG_FILE"
            exit 1
        }
    fi
    
    # Si c'était dans un DMG, recréer le DMG
    if [ "$IS_DMG" = "1" ] && [ -n "$ORIGINAL_DMG" ]; then
        echo "   🔨 Reconstruction du DMG..."
        DMG_TEMP="/tmp/fix_dmg_content_$$"
        rm -rf "$DMG_TEMP" && mkdir -p "$DMG_TEMP"
        # Utiliser le nom original du PKG trouvé dans le DMG
        if [ -n "$ORIGINAL_PKG_NAME" ]; then
            cp "$PKG_FILE" "$DMG_TEMP/$ORIGINAL_PKG_NAME"
        else
            # Fallback: extraire le nom depuis le Distribution.xml
            PKG_NAME_FROM_DIST=$(grep -oP 'CPACK_PACKAGE_FILE_NAME|#\K[^<]+' "$TEMP_DIR/Distribution" | head -1 | sed 's/\.pkg$//').pkg
            if [ -n "$PKG_NAME_FROM_DIST" ] && [ "$PKG_NAME_FROM_DIST" != ".pkg" ]; then
                cp "$PKG_FILE" "$DMG_TEMP/$PKG_NAME_FROM_DIST"
            else
                # Dernier recours: utiliser le nom du DMG
                cp "$PKG_FILE" "$DMG_TEMP/$(basename "$ORIGINAL_DMG" .dmg).pkg"
            fi
        fi
        
        # Copier aussi l'app de désinstallation si elle existe
        MOUNT_POINT="/tmp/fix_dmg_copy_$$"
        hdiutil attach -quiet "$ORIGINAL_DMG" -mountpoint "$MOUNT_POINT" 2>/dev/null
        if [ -d "$MOUNT_POINT/UninstallComposeSiren_Orchestra.app" ]; then
            cp -R "$MOUNT_POINT/UninstallComposeSiren_Orchestra.app" "$DMG_TEMP/"
        fi
        hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
        
        # Créer le nouveau DMG
        NEW_DMG="/tmp/fix_dmg_new_$$.dmg"
        rm -f "$NEW_DMG"
        hdiutil create -ov -volname "$(basename "$ORIGINAL_DMG" .dmg)" -fs HFS+ -srcfolder "$DMG_TEMP" "$NEW_DMG" || {
            echo "   ❌ Erreur lors de la création du DMG"
            rm -rf "$TEMP_DIR" "$DMG_TEMP" "$PKG_FILE"
            exit 1
        }
        # Convertir et remplacer l'original
        BACKUP_DMG="${ORIGINAL_DMG}.backup"
        cp "$ORIGINAL_DMG" "$BACKUP_DMG"
        rm -f "$ORIGINAL_DMG"
        hdiutil convert -format UDZO -o "$ORIGINAL_DMG" "$NEW_DMG" || {
            echo "   ❌ Erreur lors de la conversion du DMG, restauration de la sauvegarde"
            mv "$BACKUP_DMG" "$ORIGINAL_DMG"
            rm -rf "$TEMP_DIR" "$DMG_TEMP" "$PKG_FILE" "$NEW_DMG"
            exit 1
        }
        rm -f "$BACKUP_DMG"
        rm -rf "$DMG_TEMP" "$NEW_DMG" "$PKG_FILE"
        echo "   ✅ DMG reconstruit avec succès"
    fi
    
    rm -rf "$TEMP_DIR"
    [ "$IS_DMG" = "0" ] && rm -f "$PKG_FILE"
    echo "✅ PKG modifié avec succès"
else
    echo "❌ Distribution.xml non trouvé dans le PKG"
    [ "$IS_DMG" = "1" ] && hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
    rm -rf "$TEMP_DIR"
    exit 1
fi

