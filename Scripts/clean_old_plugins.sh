#!/bin/bash
echo "🧹 Nettoyage des anciennes versions ComposeSiren"
echo "================================================"
echo ""

# Liste des emplacements à supprimer
LOCATIONS=(
    "/Library/Audio/Plug-Ins/Components/ComposeSiren.component"
    "/Library/Audio/Plug-Ins/Components/ComposeSiren-Fixed.component"
    "$HOME/Library/Audio/Plug-Ins/Components/ComposeSiren.component"
    "$HOME/Library/Audio/Plug-Ins/Components/ComposeSiren-Fixed.component"
    "/Library/Audio/Plug-Ins/VST3/ComposeSiren.vst3"
    "/Library/Audio/Plug-Ins/VST3/ComposeSiren-Fixed.vst3"
    "$HOME/Library/Audio/Plug-Ins/VST3/ComposeSiren.vst3"
    "$HOME/Library/Audio/Plug-Ins/VST3/ComposeSiren-Fixed.vst3"
    "/Library/Audio/Plug-Ins/VST3/ComposeSirenDDSP.vst3"
    "/Library/Audio/Plug-Ins/Mecanique Vivante/ComposeSiren"
)

# Compter combien existent
COUNT=0
for loc in "${LOCATIONS[@]}"; do
    if [ -e "$loc" ]; then
        ((COUNT++))
        echo "  ✓ Trouvé: $loc"
    fi
done

echo ""
echo "Total: $COUNT fichier(s)/dossier(s) à supprimer"
echo ""
read -p "Confirmer la suppression ? (oui/non) : " CONFIRM

if [ "$CONFIRM" != "oui" ]; then
    echo "Annulé."
    exit 0
fi

echo ""
echo "Suppression en cours..."

# Supprimer sans sudo d'abord (dossier utilisateur)
for loc in "${LOCATIONS[@]}"; do
    if [[ "$loc" == "$HOME"* ]] && [ -e "$loc" ]; then
        echo "  🗑️  $loc"
        rm -rf "$loc"
    fi
done

# Supprimer avec sudo (dossiers système)
echo ""
echo "Les emplacements système nécessitent sudo..."
sudo -v

for loc in "${LOCATIONS[@]}"; do
    if [[ "$loc" != "$HOME"* ]] && [ -e "$loc" ]; then
        echo "  🗑️  $loc"
        sudo rm -rf "$loc"
    fi
done

echo ""
echo "✅ Nettoyage terminé!"
echo ""
echo "💡 Conseil: Relancez votre DAW ou exécutez:"
echo "   killall -9 AudioComponentRegistrar"




