#!/bin/bash
# Script pour publier une release sur GitHub
# Usage: ./scripts/publish_release.sh [version] [--skip-git] [--skip-gh]
#
# Ce script:
# 1. Trouve le DMG notarisé
# 2. Vérifie l'état git
# 3. Crée un tag git
# 4. Pousse sur git
# 5. Crée une release GitHub avec le DMG

set -e

VERSION=${1:-""}
SKIP_GIT=false
SKIP_GH=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        --skip-git)
            SKIP_GIT=true
            ;;
        --skip-gh)
            SKIP_GH=true
            ;;
    esac
done

if [ -z "$VERSION" ]; then
    echo "❌ Version requise"
    echo "Usage: $0 [version] [--skip-git] [--skip-gh]"
    echo "Exemple: $0 2.0.0"
    exit 1
fi

echo "============================================"
echo "🚀 Publication Release ComposeSiren"
echo "Version: ${VERSION}"
echo "============================================"
echo ""

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# 1. Trouver le DMG notarisé
print_status "Recherche du DMG notarisé..."

# Chercher dans plusieurs emplacements possibles
DMG_FILE=""
POSSIBLE_LOCATIONS=(
    "build/Packaging/ComposeSiren_Installer_artefacts"
    "build/Packaging/ComposeSiren_Orchestra_Installer_artefacts"
    "Releases/v${VERSION}-macOS"
    "Releases"
)

for location in "${POSSIBLE_LOCATIONS[@]}"; do
    if [ -d "$location" ] || [ -f "$location" ]; then
        found=$(find "$location" -name "*.dmg" -type f 2>/dev/null | head -1)
        if [ -n "$found" ] && [ -f "$found" ]; then
            DMG_FILE="$found"
            break
        fi
    fi
done

if [ -z "$DMG_FILE" ] || [ ! -f "$DMG_FILE" ]; then
    print_error "Aucun DMG trouvé pour la version ${VERSION}"
    echo ""
    echo "Emplacements recherchés:"
    for loc in "${POSSIBLE_LOCATIONS[@]}"; do
        echo "  - ${loc}"
    done
    echo ""
    echo "Vérifiez que le DMG notarisé existe ou exécutez d'abord:"
    echo "  ./scripts/build_release_macos.sh ${VERSION}"
    exit 1
fi

print_success "DMG trouvé: ${DMG_FILE}"
echo ""

# Vérifier la notarisation
print_status "Vérification de la notarisation..."
if spctl --assess --verbose "${DMG_FILE}" 2>&1 | grep -q "accepted"; then
    print_success "DMG notarisé et accepté par Gatekeeper"
else
    print_warning "DMG non notarisé ou rejeté par Gatekeeper"
    read -p "Continuer quand même ? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi
echo ""

# Préparer le DMG pour la release
RELEASES_DIR="Releases"
VERSION_DIR="${RELEASES_DIR}/v${VERSION}-macOS"
mkdir -p "${VERSION_DIR}"

DMG_NAME="ComposeSiren-v${VERSION}-macOS.dmg"
RELEASE_DMG="${VERSION_DIR}/${DMG_NAME}"

if [ "$DMG_FILE" != "$RELEASE_DMG" ]; then
    print_status "Copie du DMG vers le dossier de release..."
    cp "${DMG_FILE}" "${RELEASE_DMG}"
    print_success "DMG copié vers: ${RELEASE_DMG}"
else
    print_success "DMG déjà dans le dossier de release"
fi

DMG_SIZE=$(du -h "${RELEASE_DMG}" | cut -f1)
print_status "Taille du DMG: ${DMG_SIZE}"
echo ""

# 2. Vérifier l'état git
if [ "$SKIP_GIT" = false ]; then
    print_status "Vérification de l'état git..."
    
    # Vérifier qu'on est dans un repo git
    if ! git rev-parse --git-dir > /dev/null 2>&1; then
        print_error "Ce n'est pas un dépôt git"
        exit 1
    fi
    
    # Vérifier qu'il n'y a pas de changements non commités
    if ! git diff-index --quiet HEAD --; then
        print_warning "Il y a des changements non commités"
        git status --short
        read -p "Continuer quand même ? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
    
    # Vérifier la branche actuelle
    CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    print_status "Branche actuelle: ${CURRENT_BRANCH}"
    
    # Vérifier si le tag existe déjà
    if git rev-parse "v${VERSION}" >/dev/null 2>&1; then
        print_warning "Le tag v${VERSION} existe déjà"
        read -p "Le supprimer et le recréer ? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git tag -d "v${VERSION}" 2>/dev/null || true
            git push origin ":refs/tags/v${VERSION}" 2>/dev/null || true
        else
            print_error "Tag existant, arrêt"
            exit 1
        fi
    fi
    
    # Créer le tag
    print_status "Création du tag v${VERSION}..."
    git tag -a "v${VERSION}" -m "Release v${VERSION}"
    print_success "Tag créé: v${VERSION}"
    echo ""
    
    # Pousser les commits et le tag
    print_status "Poussage vers git..."
    read -p "Pousser les commits et le tag vers origin ? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        git push origin "${CURRENT_BRANCH}"
        git push origin "v${VERSION}"
        print_success "Poussé vers origin"
    else
        print_warning "Push annulé. Vous pouvez le faire manuellement:"
        echo "  git push origin ${CURRENT_BRANCH}"
        echo "  git push origin v${VERSION}"
    fi
    echo ""
else
    print_status "Étape git ignorée (--skip-git)"
    echo ""
fi

# 3. Créer la release GitHub
if [ "$SKIP_GH" = false ]; then
    print_status "Création de la release GitHub..."
    
    # Vérifier que gh CLI est installé
    if ! command -v gh &> /dev/null; then
        print_error "GitHub CLI (gh) n'est pas installé"
        echo "Installez-le avec: brew install gh"
        echo "Ou créez la release manuellement sur GitHub"
        exit 1
    fi
    
    # Vérifier l'authentification
    if ! gh auth status &> /dev/null; then
        print_error "Non authentifié avec GitHub CLI"
        echo "Exécutez: gh auth login"
        exit 1
    fi
    
    # Configurer le dépôt par défaut si nécessaire
    print_status "Vérification de la configuration du dépôt GitHub..."
    if ! gh repo view &> /dev/null; then
        # Extraire le repo depuis git remote
        GIT_REMOTE=$(git remote get-url origin 2>/dev/null || git config --get remote.origin.url)
        if [[ "$GIT_REMOTE" =~ github.com[:/]([^/]+)/([^/]+)\.git ]]; then
            REPO_OWNER="${BASH_REMATCH[1]}"
            REPO_NAME="${BASH_REMATCH[2]%.git}"
            print_status "Configuration du dépôt par défaut: ${REPO_OWNER}/${REPO_NAME}"
            gh repo set-default "${REPO_OWNER}/${REPO_NAME}" || {
                print_error "Impossible de configurer le dépôt par défaut"
                echo "Exécutez manuellement: gh repo set-default ${REPO_OWNER}/${REPO_NAME}"
                exit 1
            }
        else
            print_error "Impossible de déterminer le dépôt GitHub depuis git remote"
            echo "Remote actuel: ${GIT_REMOTE}"
            echo "Exécutez manuellement: gh repo set-default OWNER/REPO"
            exit 1
        fi
    else
        print_success "Dépôt GitHub configuré"
    fi
    echo ""
    
    # Chercher les notes de release
    RELEASE_NOTES=""
    NOTES_FILES=(
        "Releases/RELEASE_NOTES_v${VERSION}.md"
        "Releases/RELEASE_NOTES_v${VERSION}-full.md"
        "Releases/RELEASE_NOTES_v${VERSION%.*}.md"
    )
    
    for notes_file in "${NOTES_FILES[@]}"; do
        if [ -f "$notes_file" ]; then
            RELEASE_NOTES="$notes_file"
            print_success "Notes de release trouvées: ${notes_file}"
            break
        fi
    done
    
    if [ -z "$RELEASE_NOTES" ]; then
        print_warning "Aucune note de release trouvée"
        RELEASE_NOTES="Release v${VERSION}"
    fi
    
    # Créer la release
    print_status "Création de la release GitHub v${VERSION}..."
    
    if [ -f "$RELEASE_NOTES" ]; then
        gh release create "v${VERSION}" \
            --title "ComposeSiren v${VERSION}" \
            --notes-file "$RELEASE_NOTES" \
            "${RELEASE_DMG}"
    else
        gh release create "v${VERSION}" \
            --title "ComposeSiren v${VERSION}" \
            --notes "${RELEASE_NOTES}" \
            "${RELEASE_DMG}"
    fi
    
    if [ $? -eq 0 ]; then
        print_success "Release GitHub créée avec succès!"
        echo ""
        print_status "URL de la release:"
        gh release view "v${VERSION}" --web
    else
        print_error "Échec de la création de la release GitHub"
        exit 1
    fi
else
    print_status "Étape GitHub ignorée (--skip-gh)"
    echo ""
    print_status "Pour créer la release manuellement:"
    echo "  gh release create v${VERSION} --title 'ComposeSiren v${VERSION}' --notes-file Releases/RELEASE_NOTES_v${VERSION}.md ${RELEASE_DMG}"
fi

echo ""
echo "============================================"
print_success "Publication terminée avec succès!"
echo "============================================"
echo ""
echo "📦 Fichier release: ${RELEASE_DMG}"
echo "📊 Taille: ${DMG_SIZE}"
echo "🏷️  Tag: v${VERSION}"
echo ""

