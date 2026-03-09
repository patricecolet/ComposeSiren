#!/bin/bash
# Script pour construire seulement Orchestra (sans Solo) avec un PKG correct dès le build

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"

echo "🔧 Construction de ComposeSiren Orchestra uniquement (sans Solo)"
echo ""

# Configuration CMake avec l'option BUILD_ONLY_ORCHESTRA
BUILD_DIR="build_orchestra_only"

echo "📦 Configuration CMake..."
cmake -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_ONLY_ORCHESTRA=ON \
  "$@"

echo ""
echo "🔨 Compilation..."
cmake --build "$BUILD_DIR" --config Release

echo ""
echo "✅ Build terminé"
echo ""
echo "📦 Le PKG devrait être dans:"
echo "   $BUILD_DIR/Packaging/ComposeSiren_Orchestra_Installer_artefacts/"
echo ""
echo "💡 Pour vérifier que les PackageInfo sont corrects:"
echo "   ./scripts/check_pkg_binaries.sh $BUILD_DIR/Packaging/ComposeSiren_Orchestra_Installer_artefacts/*.pkg"



