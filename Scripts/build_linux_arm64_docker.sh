#!/bin/bash
# Build ComposeSiren .deb pour Linux ARM64 (Raspberry Pi) via Docker
#
# Compile sur Mac dans un conteneur Linux ARM64, lance cpack, produit un .deb.
# Prérequis: Docker Desktop
#
# Usage:
#   cd /path/to/ComposeSiren-fixed
#   ./scripts/build_linux_arm64_docker.sh

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "🔧 Build .deb ComposeSiren Linux ARM64 via Docker"
echo "=================================================="
echo ""

if ! command -v docker >/dev/null 2>&1; then
    echo "❌ Docker non installé ou pas dans le PATH."
    echo "   Installez Docker Desktop: https://www.docker.com/products/docker-desktop"
    exit 1
fi

if ! docker info >/dev/null 2>&1; then
    echo "❌ Docker ne répond pas. Démarrez Docker Desktop."
    exit 1
fi

echo "📦 Construction de l'image Docker (linux/arm64)..."
docker build \
    --platform linux/arm64 \
    -f docker/Dockerfile.linux-arm64 \
    -t composesiren-build:linux-arm64 \
    .

echo ""
echo "🏗️ Build + cpack (incrémentale : réutilise build/). Clean : rm -rf build puis relancer."
echo ""
docker run --rm \
    --platform linux/arm64 \
    -v "$REPO_ROOT:/src:rw" \
    -w /src \
    composesiren-build:linux-arm64

echo ""
DEB_DIR="$REPO_ROOT/build/Packaging/ComposeSiren_Orchestra_Installer_artefacts"
DEB=$(find "$DEB_DIR" -maxdepth 1 -name "*.deb" -type f 2>/dev/null | head -1)

if [ -n "$DEB" ] && [ -f "$DEB" ]; then
    echo "✅ .deb généré:"
    echo "   $DEB"
    echo ""
    echo "Sur le Raspberry Pi:"
    echo "  scp \"$DEB\" user@raspberry:~/"
    echo "  ssh user@raspberry \"sudo dpkg -i ~/$(basename "$DEB")\""
    echo ""
    echo "Ou: sudo dpkg -i $(basename "$DEB")  (sur le Pi après copie)"
else
    echo "⚠️ Aucun .deb trouvé dans $DEB_DIR"
    ls -la "$DEB_DIR" 2>/dev/null || true
    exit 1
fi
