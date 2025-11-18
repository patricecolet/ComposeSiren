#!/bin/bash
# Script pour compiler un plugin Solo (Standalone) à partir de l'architecture multi-modèles

set -e

PROJECT_ROOT="/Users/patricecolet/repo/ComposeSiren-fixed"
BUILD_DIR="$PROJECT_ROOT/build-solo"
MODEL=${1:-S1}
TARGET="ComposeSiren_${MODEL}"

echo "🔨 Compilation du plugin ${TARGET} (Standalone)"
echo "===================================================="

mkdir -p "$BUILD_DIR"
cd "$PROJECT_ROOT"

echo "⚙️  Configuration CMake..."
if ! cmake -S . -B "$BUILD_DIR"; then
    echo "❌ Échec de la configuration CMake (bug JUCE macOS 15 - CGWindowListCreateImage)."
    echo "   Compilation impossible pour l'instant."
    exit 1
fi

echo "⚙️  Compilation..."
if ! cmake --build "$BUILD_DIR" --target "$TARGET" --config Debug; then
    echo "❌ Échec de la compilation (bug JUCE macOS 15 - CGWindowListCreateImage)."
    echo "   Utiliser un macOS < 15 ou attendre un correctif JUCE."
    exit 1
fi

APP_PATH="$BUILD_DIR/${TARGET}_artefacts/Debug/Standalone/${TARGET}.app"
if [ -d "$APP_PATH" ]; then
    echo ""
    echo "✅ Compilation Standalone réussie!"
    echo "📍 Application: $APP_PATH"
    echo ""
    echo "🚀 Lancement de l'application Standalone..."
    open "$APP_PATH"
else
    echo "⚠️ Compilation terminée mais application introuvable ($APP_PATH)"
fi


