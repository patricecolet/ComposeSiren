#!/bin/bash
# Script de compilation manuelle pour ComposeSiren S1 Alto
# Ce script compile le plugin en utilisant les mêmes flags que le projet Xcode

set -e

echo "🔨 Compilation manuelle de ComposeSiren S1 Alto"
echo "================================================"
echo ""

PROJECT_ROOT="/Users/patricecolet/repo/ComposeSiren-fixed"
BUILD_DIR="$PROJECT_ROOT/build-s1-manual"
SOURCE_DIR="$PROJECT_ROOT/Source"

# Créer le dossier de build
mkdir -p "$BUILD_DIR"

echo "📂 Sources :"
echo "  Common: $SOURCE_DIR/Common/"
echo "  Solo S1: $SOURCE_DIR/Solo/S1/"
echo ""

# Liste des fichiers source
SOURCES_COMMON=(
    "$SOURCE_DIR/Common/CS_midiIN.cpp"
    "$SOURCE_DIR/Common/Sirene.cpp"
    "$SOURCE_DIR/Common/synth.cpp"
    "$SOURCE_DIR/Common/allpass.cpp"
    "$SOURCE_DIR/Common/comb.cpp"
    "$SOURCE_DIR/Common/mareverbe.cpp"
)

SOURCES_S1=(
    "$SOURCE_DIR/Solo/S1/PluginProcessor_S1.cpp"
    "$SOURCE_DIR/Solo/S1/PluginEditor_S1.cpp"
)

JUCE_MODULES=(
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_audio_basics.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_audio_devices.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_audio_formats.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_audio_processors.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_audio_utils.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_core.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_data_structures.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_events.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_graphics.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_gui_basics.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_gui_extra.mm"
    "$PROJECT_ROOT/JuceLibraryCode/include_juce_audio_plugin_client_Standalone.cpp"
)

# Flags de compilation
CXXFLAGS=(
    -std=c++20
    -stdlib=libc++
    -O0
    -g
    -DDEBUG=1
    -D_DEBUG=1
    -DSOLO_PLUGIN=1
    -DSIREN_MODEL=\"S1\"
    -DSIREN_DISPLAY_NAME=\"S1\ Alto\"
    -DCMS_BUILD_WITH_PROJUCER=1
    -DJucePlugin_Build_Standalone=1
    -DJucePlugin_Build_AU=1
    -DJucePlugin_Build_VST3=0
    -I"$PROJECT_ROOT/Dependencies/JUCE/modules"
    -I"$PROJECT_ROOT/JuceLibraryCode"
    -I"$SOURCE_DIR/Common"
    -I"$SOURCE_DIR/Solo/S1"
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

FRAMEWORKS=(
    -framework Cocoa
    -framework CoreAudio
    -framework CoreMIDI
    -framework AudioToolbox
    -framework AudioUnit
    -framework Carbon
    -framework Accelerate
    -framework QuartzCore
    -framework IOKit
)

echo "⚙️  Compilation des fichiers..."

# Compiler tous les fichiers
ALL_SOURCES=("${SOURCES_COMMON[@]}" "${SOURCES_S1[@]}" "${JUCE_MODULES[@]}")
OBJECTS=()

for src in "${ALL_SOURCES[@]}"; do
    if [ ! -f "$src" ]; then
        echo "❌ Fichier manquant: $src"
        exit 1
    fi
    
    filename=$(basename "$src")
    objname="${filename%.*}.o"
    objpath="$BUILD_DIR/$objname"
    
    echo "  Compiling $filename..."
    
    if [[ "$src" == *.mm ]]; then
        clang++ "${CXXFLAGS[@]}" -x objective-c++ -c "$src" -o "$objpath" 2>&1 | grep -v "warning:" || true
    else
        clang++ "${CXXFLAGS[@]}" -c "$src" -o "$objpath" 2>&1 | grep -v "warning:" || true
    fi
    
    if [ $? -ne 0 ]; then
        echo "❌ Erreur de compilation: $filename"
        exit 1
    fi
    
    OBJECTS+=("$objpath")
done

echo ""
echo "🔗 Liaison..."

# Créer l'exécutable Standalone
clang++ "${OBJECTS[@]}" "${FRAMEWORKS[@]}" -o "$BUILD_DIR/ComposeSirenS1" 2>&1 | grep -v "warning:" || true

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Compilation réussie!"
    echo "📍 Exécutable: $BUILD_DIR/ComposeSirenS1"
    echo ""
    echo "Pour tester:"
    echo "  $BUILD_DIR/ComposeSirenS1"
else
    echo ""
    echo "❌ Erreur lors de la liaison"
    exit 1
fi

