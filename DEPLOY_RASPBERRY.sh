#!/bin/bash
# Script de déploiement ComposeSiren sur Raspberry Pi
# USAGE: cd ~/ComposeSiren-fixed && bash DEPLOY_RASPBERRY.sh
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
echo -e "${YELLOW}Étape 1/5: Installation des dépendances${NC}"
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
    echo "Usage: cd ~/ComposeSiren-fixed && bash DEPLOY_RASPBERRY.sh"
    exit 1
fi

# Mettre à jour le repo
git pull origin master
git submodule update --init --recursive

echo -e "${GREEN}✓ Code mis à jour${NC}"
echo ""

# Étape 3: Configurer le build
echo -e "${YELLOW}Étape 3/6: Configuration CMake${NC}"
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
cpack

echo -e "${GREEN}✓ Package créé${NC}"
echo ""

# Étape 6: Installation du package
echo -e "${YELLOW}Étape 6/6: Installation du package${NC}"

# Trouver le package créé
DEB_PACKAGE=$(find Packaging/ComposeSiren_Installer_artefacts -name "*.deb" | head -1)

if [ ! -f "$DEB_PACKAGE" ]; then
    echo -e "${RED}✗ Erreur: package .deb non trouvé${NC}"
    exit 1
fi

echo "Package trouvé: $DEB_PACKAGE"
sudo dpkg -i "$DEB_PACKAGE"

echo -e "${GREEN}✓ Installation terminée${NC}"
echo ""

# Vérification
echo "🧪 Vérification de l'installation..."
which ComposeSiren
dpkg -L composesiren | grep bin

echo ""
echo -e "${GREEN}════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ Déploiement terminé avec succès !${NC}"
echo -e "${GREEN}════════════════════════════════════════${NC}"
echo ""
echo "📋 Corrections appliquées dans cette version:"
echo "  ✓ Pan (CC10) sur canaux 1-7"
echo "  ✓ Reverb (CC64-70) sur canal 16"
echo "  ✓ Synchronisation MIDI → Interface UI"
echo ""
echo "🎹 Test des contrôles MIDI:"
echo "  Canal 1-7, CC10  → Pan gauche/droite"
echo "  Canal 16, CC64   → Enable Reverb (≥64)"
echo "  Canal 16, CC65   → Room Size"
echo "  Canal 16, CC66   → Dry/Wet"
echo "  Canal 16, CC67   → Damping"
echo "  Canal 16, CC68   → Highpass Filter"
echo "  Canal 16, CC69   → Lowpass Filter"
echo "  Canal 16, CC70   → Width"
echo ""
echo "🚀 Pour lancer: ComposeSiren"

