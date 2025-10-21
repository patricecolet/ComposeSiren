# 🚀 Déploiement rapide sur Raspberry Pi

## Version corrigée: Pan + Reverb fonctionnels

**Date:** 21 Octobre 2025  
**Corrections:** CC10 (Pan) + CC64-70 (Reverb) + Sync UI MIDI

---

## Méthode 1: Script automatique (recommandé)

### Sur le Raspberry Pi:

```bash
# 1. Cloner le repo (si pas déjà fait)
cd ~
git clone --recursive git@github.com:patricecolet/ComposeSiren.git ComposeSiren-fixed
cd ComposeSiren-fixed

# 2. Transférer le script depuis le Mac
# (depuis le Mac:)
scp DEPLOY_RASPBERRY.sh sirenateur@<IP_RASPBERRY>:~/ComposeSiren-fixed/

# 3. Sur le Raspberry, exécuter DEPUIS le repo
ssh sirenateur@<IP_RASPBERRY>
cd ~/ComposeSiren-fixed
bash DEPLOY_RASPBERRY.sh
```

Le script fait tout automatiquement :
- ✅ Installe les dépendances
- ✅ Met à jour le code (git pull)
- ✅ Compile (30-60 min)
- ✅ Crée et installe le package .deb

---

## Méthode 2: Étapes manuelles

### 1. Installer les dépendances

```bash
sudo apt-get update
sudo apt-get install -y \
    git cmake build-essential \
    libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libfreetype-dev libasound2-dev \
    libgl1-mesa-dev libglu1-mesa-dev
```

### 2. Récupérer le code

```bash
cd ~
git clone --recursive git@github.com:patricecolet/ComposeSiren.git ComposeSiren-fixed
cd ComposeSiren-fixed
```

### 3. Compiler et créer le package

```bash
mkdir -p build-raspberry
cd build-raspberry
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j2
cpack
```

### 4. Installer le package .deb

```bash
sudo dpkg -i Packaging/ComposeSiren_Installer_artefacts/composesiren_*.deb
```

---

## Vérification

```bash
# Vérifier l'installation
which ComposeSiren
# → /usr/bin/ComposeSiren

dpkg -l | grep composesiren
# → ii  composesiren  1.5.0  arm64  ComposeSiren audio synthesizer

# Lancer (Ctrl+C pour quitter)
ComposeSiren
```

### Désinstaller si besoin

```bash
sudo dpkg -r composesiren
```

---

## Intégration avec start-raspberry.sh

Le script `start-raspberry.sh` utilisera automatiquement la nouvelle version car il cherche `ComposeSiren` dans le PATH.

```bash
# Dans start-raspberry.sh (déjà présent):
if command -v ComposeSiren >/dev/null 2>&1; then
    export DISPLAY=:0
    ComposeSiren &
    echo "✅ ComposeSiren démarré avec corrections Pan + Reverb"
fi
```

---

## Tests MIDI

Une fois lancé, testez avec votre contrôleur MIDI :

### Pan (CC10) - Canaux 1-7
```
Canal 1, CC10 = 0   → Sirène S1 à gauche
Canal 1, CC10 = 64  → Sirène S1 au centre  
Canal 1, CC10 = 127 → Sirène S1 à droite
```

### Reverb - Canal 16
```
Canal 16, CC64 = 127  → Activer reverb
Canal 16, CC65 = 64   → Room Size 50%
Canal 16, CC66 = 64   → Dry/Wet 50%
Canal 16, CC67 = 64   → Damping 50%
Canal 16, CC68 = 64   → Highpass ~1kHz
Canal 16, CC69 = 64   → Lowpass ~11kHz  
Canal 16, CC70 = 64   → Width 50%
```

**L'interface doit maintenant réagir en temps réel aux changements MIDI !** 🎹✨

---

## Dépannage

### Compilation échoue (Out of memory)
```bash
# Utiliser 1 seul cœur
cmake --build . --config Release -j1

# Ou augmenter le swap
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile  # CONF_SWAPSIZE=2048
sudo dphys-swapfile setup && sudo dphys-swapfile swapon
```

### Audio ne sort pas
```bash
# Vérifier la carte son
aplay -l

# Tester
speaker-test -c 2 -t wav

# Créer ~/.asoundrc si besoin
cat > ~/.asoundrc << 'EOF'
pcm.!default {
    type hw
    card 0
    device 0
}
EOF
```

### JUCE submodule manquant
```bash
cd ~/ComposeSiren-fixed
git submodule update --init --recursive
```

---

## 📞 Support

Voir `RASPBERRY_COMPILE.md` pour plus de détails.

**Commit actuel:** 40d651a (Pan + Reverb fixes)

