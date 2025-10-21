#!/bin/bash
# Script d'aide pour configurer la signature et notarisation macOS

set -e

echo "============================================"
echo "Configuration de la signature macOS"
echo "============================================"
echo ""

# Vérifier qu'on est sur macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ Ce script ne fonctionne que sur macOS"
    exit 1
fi

# Vérifier si LocalConfig.cmake existe déjà
if [ -f "LocalConfig.cmake" ]; then
    echo "⚠️  LocalConfig.cmake existe déjà"
    read -p "Voulez-vous le recréer ? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Configuration annulée"
        exit 0
    fi
fi

echo "Étape 1/4 : Vérification des certificats Developer ID..."
echo ""

# Lister les certificats disponibles
echo "Certificats trouvés dans votre Keychain :"
echo "=========================================="
security find-identity -v -p codesigning | grep "Developer ID"
echo ""

# Demander les certificats
echo "Étape 2/4 : Configuration des certificats..."
echo ""
echo "Copiez-collez le nom COMPLET (avec guillemets) du certificat 'Application' :"
read -r CERT_APP
echo ""
echo "Copiez-collez le nom COMPLET (avec guillemets) du certificat 'Installer' :"
read -r CERT_INSTALLER
echo ""

# Configuration de la notarisation
echo "Étape 3/4 : Configuration de la notarisation..."
echo ""
echo "Avez-vous déjà configuré un profil notarytool ? (y/N)"
read -r -n 1 HAS_PROFILE
echo ""

PROFILE_NAME="notarytool-profile"

if [[ $HAS_PROFILE =~ ^[Yy]$ ]]; then
    echo "Entrez le nom du profil keychain (défaut: notarytool-profile) :"
    read -r CUSTOM_PROFILE
    if [ ! -z "$CUSTOM_PROFILE" ]; then
        PROFILE_NAME="$CUSTOM_PROFILE"
    fi
else
    echo ""
    echo "Vous devez configurer la notarisation manuellement :"
    echo "  1. Créez un App-Specific Password sur https://appleid.apple.com"
    echo "  2. Exécutez :"
    echo ""
    echo "     xcrun notarytool store-credentials \"$PROFILE_NAME\" \\"
    echo "       --apple-id \"votre.email@example.com\" \\"
    echo "       --team-id \"VOTRE_TEAM_ID\" \\"
    echo "       --password \"xxxx-xxxx-xxxx-xxxx\""
    echo ""
    read -p "Appuyez sur Entrée une fois la configuration terminée..."
fi

# Activer la notarisation
echo ""
echo "Étape 4/4 : Activation de la notarisation..."
echo ""
echo "Voulez-vous activer la notarisation automatique ? (Y/n)"
echo "(La notarisation ajoute 5-30 minutes au temps de build)"
read -r -n 1 ENABLE_NOTARIZATION
echo ""

if [[ $ENABLE_NOTARIZATION =~ ^[Nn]$ ]]; then
    NOTARIZATION_VALUE="OFF"
else
    NOTARIZATION_VALUE="ON"
fi

# Créer LocalConfig.cmake
echo ""
echo "Création de LocalConfig.cmake..."

cat > LocalConfig.cmake <<EOF
# Configuration générée par setup_codesigning.sh
# Date : $(date)

# Certificat pour signer les applications (.app)
set(APPLE_DEVELOPER_ID_APPLICATION $CERT_APP)

# Certificat pour signer les installers (.pkg)
set(APPLE_DEVELOPER_ID_INSTALLER $CERT_INSTALLER)

# Profil keychain pour la notarisation
set(APPLE_NOTARIZATION_KEYCHAIN_PROFILE "$PROFILE_NAME")

# Activer la notarisation
set(ENABLE_NOTARIZATION $NOTARIZATION_VALUE)
EOF

echo ""
echo "✅ Configuration terminée !"
echo ""
echo "LocalConfig.cmake a été créé avec succès."
echo ""
echo "Prochaines étapes :"
echo "  1. Nettoyez le build précédent : rm -rf build"
echo "  2. Configurez CMake : cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "  3. Buildez : cmake --build build --config Release"
echo ""
echo "Le DMG signé et notarisé sera dans :"
echo "  build/Packaging/ComposeSiren_Installer_artefacts/"
echo ""
echo "Pour plus d'informations, consultez SIGNATURE_ET_NOTARISATION.md"



