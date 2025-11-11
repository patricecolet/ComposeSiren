#!/bin/bash
# Script pour compiler et tester le plugin S1 Solo

set -e

PROJECT_ROOT="/Users/patricecolet/repo/ComposeSiren-fixed"
SOURCE_DIR="$PROJECT_ROOT/Source"

echo "🔨 Compilation du plugin ComposeSiren S1 Alto Solo"
echo "===================================================="
echo ""

# Sauvegarder les liens actuels
echo "💾 Sauvegarde de la configuration Orchestra..."
ORCHESTRA_LINKS=true

# Basculer vers S1
echo "🔀 Basculement vers les sources S1..."
cd "$SOURCE_DIR"

rm -f PluginProcessor.cpp PluginProcessor.h PluginEditor.cpp PluginEditor.h

ln -sf Solo/S1/PluginProcessor_S1.cpp PluginProcessor.cpp
ln -sf Solo/S1/PluginProcessor_S1.h PluginProcessor.h
ln -sf Solo/S1/PluginEditor_S1.cpp PluginEditor.cpp
ln -sf Solo/S1/PluginEditor_S1.h PluginEditor.h

echo "✓ Liens symboliques mis à jour vers S1"
ls -l Plugin*.cpp Plugin*.h | awk '{print "  " $9 " -> " $11}'
echo ""

# Compiler avec Xcode
echo "⚙️  Compilation avec Xcode..."
echo ""

cd "$PROJECT_ROOT/Builds/MacOSX"

# Nettoyer le build précédent
rm -rf build/Debug/ComposeSiren.app 2>/dev/null || true

# Compiler Standalone
echo "📦 Compilation Standalone..."
xcodebuild -project ComposeSiren.xcodeproj \
    -target "ComposeSiren - Standalone Plugin" \
    -configuration Debug \
    clean build 2>&1 | grep -E "(BUILD|error:|Compiling)" || true

if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo ""
    echo "✅ Compilation Standalone réussie!"
    echo "📍 Application: $PROJECT_ROOT/Builds/MacOSX/build/Debug/ComposeSiren.app"
    echo ""
    
    # Proposer de lancer l'application
    echo "🚀 Voulez-vous lancer l'application maintenant ?"
    echo "   $PROJECT_ROOT/Builds/MacOSX/build/Debug/ComposeSiren.app/Contents/MacOS/ComposeSiren"
    echo ""
    
    STANDALONE_SUCCESS=true
else
    echo "❌ Erreur de compilation Standalone"
    STANDALONE_SUCCESS=false
fi

# Restaurer Orchestra
echo ""
echo "🔙 Restauration de la configuration Orchestra..."
cd "$SOURCE_DIR"

rm -f PluginProcessor.cpp PluginProcessor.h PluginEditor.cpp PluginEditor.h

ln -sf Orchestra/PluginProcessor.cpp PluginProcessor.cpp
ln -sf Orchestra/PluginProcessor.h PluginProcessor.h
ln -sf Orchestra/PluginEditor.cpp PluginEditor.cpp
ln -sf Orchestra/PluginEditor.h PluginEditor.h

echo "✓ Liens symboliques restaurés vers Orchestra"
echo ""

if [ "$STANDALONE_SUCCESS" = true ]; then
    echo "=================================================="
    echo "✅ Plugin S1 compilé avec succès!"
    echo "=================================================="
    echo ""
    echo "Pour tester:"
    echo "  open $PROJECT_ROOT/Builds/MacOSX/build/Debug/ComposeSiren.app"
    echo ""
    echo "Note: L'application s'appelle encore 'ComposeSiren' mais"
    echo "      elle utilise le code du plugin S1 Solo."
else
    echo "❌ Échec de la compilation"
    exit 1
fi

