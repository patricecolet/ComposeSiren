#!/bin/bash
# Script de déploiement ComposeSiren sur Raspberry Pi
# USAGE: cd ~/ComposeSiren-fixed && bash scripts/deploy-raspberry.sh
#
# Ce script doit être exécuté DEPUIS le répertoire du repo git

set -e  # Arrêter en cas d'erreur

echo "🚀 Déploiement ComposeSiren sur Raspberry Pi"
echo "=============================================="
echo ""

# Couleurs pour les messages
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Étape 1: Vérifier les dépendances
echo -e "${YELLOW}Étape 1/6: Installation des dépendances${NC}"
sudo apt-get update
sudo apt-get install -y \
    git \
    cmake \
    build-essential \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libfreetype-dev \
    libasound2-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev

echo -e "${GREEN}✓ Dépendances installées${NC}"
echo ""

# Étape 2: Mettre à jour le code
echo -e "${YELLOW}Étape 2/6: Mise à jour du code depuis GitHub${NC}"

# Vérifier qu'on est dans un repo git
if [ ! -d ".git" ]; then
    echo -e "${RED}✗ Erreur: Ce script doit être exécuté depuis le répertoire du repo${NC}"
    echo "Usage: cd ~/ComposeSiren-fixed && bash scripts/deploy-raspberry.sh"
    exit 1
fi

# Mettre à jour le repo
git pull origin master
git submodule update --init --recursive

echo -e "${GREEN}✓ Code mis à jour${NC}"
echo ""

# Étape 3: Configurer le build
echo -e "${YELLOW}Étape 3/6: Configuration CMake${NC}"

# Nettoyer le build précédent si nécessaire
if [ -d "build-raspberry" ]; then
    echo "Nettoyage du build précédent..."
    rm -rf build-raspberry
fi

mkdir -p build-raspberry
cd build-raspberry
cmake .. -DCMAKE_BUILD_TYPE=Release

echo -e "${GREEN}✓ Configuration terminée${NC}"
echo ""

# Étape 4: Compilation
echo -e "${YELLOW}Étape 4/6: Compilation (cela peut prendre 30-60 minutes)${NC}"
echo "Utilisation de 2 cœurs pour éviter la surchauffe..."
cmake --build . --config Release -j2

echo -e "${GREEN}✓ Compilation réussie${NC}"
echo ""

# Étape 5: Création du package .deb
echo -e "${YELLOW}Étape 5/6: Création du package Debian${NC}"

# Exécuter cpack et vérifier le succès
if ! cpack; then
    echo -e "${RED}✗ Erreur lors de la création du package${NC}"
    exit 1
fi

# Lister les fichiers créés pour debug
echo "Fichiers créés dans Packaging/:"
ls -lh Packaging/ComposeSiren_Installer_artefacts/ 2>/dev/null || echo "Dossier non trouvé"

echo -e "${GREEN}✓ Package créé${NC}"
echo ""

# Étape 6: Installation du package
echo -e "${YELLOW}Étape 6/6: Installation du package${NC}"

# Trouver le package créé
DEB_PACKAGE=$(find Packaging/ComposeSiren_Installer_artefacts -name "*.deb" -type f | head -1)

if [ -z "$DEB_PACKAGE" ] || [ ! -f "$DEB_PACKAGE" ]; then
    echo -e "${RED}✗ Erreur: package .deb non trouvé${NC}"
    echo "Recherche dans: $(pwd)/Packaging/ComposeSiren_Installer_artefacts/"
    exit 1
fi

echo "Package trouvé: $DEB_PACKAGE"

# Extraire le nom du package
PACKAGE_NAME=$(basename "$DEB_PACKAGE" | cut -d'_' -f1)

# Vérifier si ComposeSiren est déjà installé
if dpkg -l | grep -q "^ii.*$PACKAGE_NAME"; then
    INSTALLED_VERSION=$(dpkg -l | grep "^ii.*$PACKAGE_NAME" | awk '{print $3}')
    echo -e "${YELLOW}⚠ ComposeSiren est déjà installé (version: $INSTALLED_VERSION)${NC}"
    echo "Désinstallation de l'ancienne version..."
    
    # Désinstaller l'ancienne version (garde les fichiers de configuration)
    if ! sudo dpkg -r "$PACKAGE_NAME"; then
        echo -e "${RED}✗ Erreur lors de la désinstallation${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Ancienne version désinstallée${NC}"
fi

# Installer le nouveau package
echo "Installation de la nouvelle version..."
if ! sudo dpkg -i "$DEB_PACKAGE"; then
    echo -e "${RED}✗ Erreur lors de l'installation du package${NC}"
    echo "Tentative de correction des dépendances..."
    if sudo apt-get install -f -y; then
        # Réessayer l'installation après correction des dépendances
        if ! sudo dpkg -i "$DEB_PACKAGE"; then
            echo -e "${RED}✗ Échec de l'installation après correction des dépendances${NC}"
            exit 1
        fi
    else
        exit 1
    fi
fi

echo -e "${GREEN}✓ Installation terminée${NC}"
echo ""

# Vérification
echo "🧪 Vérification de l'installation..."

# Vérifier la version installée
if dpkg -l | grep -q "^ii.*$PACKAGE_NAME"; then
    NEW_VERSION=$(dpkg -l | grep "^ii.*$PACKAGE_NAME" | awk '{print $3}')
    echo -e "${GREEN}✓ $PACKAGE_NAME version $NEW_VERSION installé avec succès${NC}"
else
    echo -e "${RED}✗ Le package n'apparaît pas dans la liste des packages installés${NC}"
fi

# Vérifier que l'exécutable est installé
if which ComposeSiren > /dev/null 2>&1; then
    EXEC_PATH=$(which ComposeSiren)
    echo -e "${GREEN}✓ Exécutable trouvé: $EXEC_PATH${NC}"
    
    # Vérifier que c'est un fichier exécutable
    if [ -x "$EXEC_PATH" ]; then
        echo -e "${GREEN}✓ Permissions d'exécution OK${NC}"
    else
        echo -e "${YELLOW}⚠ Fichier trouvé mais pas de permissions d'exécution${NC}"
    fi
else
    echo -e "${RED}✗ ComposeSiren non trouvé dans le PATH${NC}"
fi

# Vérifier les fichiers de ressources
RESOURCES_DIR="/usr/share/ComposeSiren/Resources"
if [ -d "$RESOURCES_DIR" ]; then
    RESOURCE_COUNT=$(ls -1 "$RESOURCES_DIR" | wc -l)
    echo -e "${GREEN}✓ Ressources trouvées: $RESOURCE_COUNT fichiers dans $RESOURCES_DIR${NC}"
else
    echo -e "${YELLOW}⚠ Dossier de ressources non trouvé: $RESOURCES_DIR${NC}"
fi

# Vérifier le .desktop
if [ -f "/usr/share/applications/composesiren.desktop" ]; then
    echo -e "${GREEN}✓ Fichier .desktop installé (icône dans le menu)${NC}"
fi

echo ""
echo -e "${GREEN}════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ Déploiement terminé avec succès !${NC}"
echo -e "${GREEN}════════════════════════════════════════${NC}"
echo ""
echo "📋 Corrections appliquées dans cette version:"
echo "  ✓ Pan (CC10) sur canaux 1-7"
echo "  ✓ Reverb (CC64-70) sur canal 16"
echo "  ✓ Synchronisation MIDI → Interface UI"
echo "  ✓ Sample rate dynamique (44.1kHz, 48kHz, 96kHz...)"
echo ""
echo "🎹 Test des contrôles MIDI:"
echo "  Canal 1-7, CC7   → Volume individuel"
echo "  Canal 1-7, CC10  → Pan gauche/droite"
echo "  Canal 1-7, CC70  → Master Volume"
echo "  Canal 1-7, CC121 → Reset canal"
echo "  Canal 16, CC64   → Enable Reverb (≥64)"
echo "  Canal 16, CC65   → Room Size"
echo "  Canal 16, CC66   → Dry/Wet"
echo "  Canal 16, CC67   → Damping"
echo "  Canal 16, CC68   → Highpass (20Hz-2kHz)"
echo "  Canal 16, CC69   → Lowpass (2kHz-20kHz)"
echo "  Canal 16, CC70   → Width stéréo"
echo "  Canal 16, CC121  → Reset ALL sirens"
echo ""
echo "🚀 Pour lancer: ComposeSiren"

