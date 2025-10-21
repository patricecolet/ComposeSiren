# Compilation ComposeSiren sur Raspberry Pi

## Corrections appliquées (21 Oct 2025)

✅ **Pan (CC10)** fonctionne maintenant sur canaux 1-7  
✅ **Reverb (CC64-70)** fonctionne maintenant sur canal 16  
✅ **Synchronisation MIDI → Interface** en temps réel  

---

## Prérequis sur Raspberry Pi

```bash
# Mise à jour du système
sudo apt-get update
sudo apt-get upgrade -y

# Dépendances JUCE pour Linux
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
```

---

## Compilation

### Étape 1: Récupérer le code depuis GitHub

```bash
# Si le repo n'existe pas encore
cd ~
git clone --recursive git@github.com:patricecolet/ComposeSiren.git ComposeSiren-fixed
cd ComposeSiren-fixed

# Ou si le repo existe déjà, mettre à jour
cd ~/ComposeSiren-fixed
git pull origin master
git submodule update --init --recursive
```

### Étape 2: Configurer avec CMake

```bash
cd ~/ComposeSiren-fixed

# Créer le dossier de build
mkdir -p build-raspberry
cd build-raspberry

# Configurer le projet
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Étape 3: Compiler

```bash
# Compiler (cela peut prendre 30-60 minutes sur Raspberry Pi)
cmake --build . --config Release -j2

# Note: -j2 utilise 2 cœurs en parallèle
# Si votre Raspberry chauffe trop, utilisez -j1
```

### Étape 4: Créer le package Debian

```bash
# Créer le package .deb
cpack

# Le package sera créé ici :
# build-raspberry/Packaging/ComposeSiren_Installer_artefacts/composesiren_1.5.0_arm64.deb
```

### Étape 5: Installer le package

```bash
# Installer avec dpkg
sudo dpkg -i Packaging/ComposeSiren_Installer_artefacts/composesiren_*.deb

# Le package installe automatiquement :
# - /usr/bin/ComposeSiren (binaire)
# - /usr/share/ComposeSiren/Resources/ (données)
# - /usr/share/applications/composesiren.desktop (menu)
# - /usr/share/pixmaps/composesiren.png (icône)
```

Maintenant le script `start-raspberry.sh` pourra trouver ComposeSiren avec `command -v ComposeSiren`.

---

## Test rapide

```bash
# Vérifier l'installation
which ComposeSiren
dpkg -l | grep composesiren

# Tester le lancement (Ctrl+C pour quitter)
ComposeSiren
```

## Désinstallation (si nécessaire)

```bash
sudo dpkg -r composesiren
```

---

## Configuration audio (HifiBerry DAC)

Le script `start-raspberry.sh` gère déjà PulseAudio, mais vérifiez que la HifiBerry est détectée :

```bash
# Lister les cartes son
aplay -l

# Tester la sortie
speaker-test -c 2 -t wav -D hw:0,0
```

Si besoin, créez `/home/sirenateur/.asoundrc` :

```
pcm.!default {
    type hw
    card 0
    device 0
}

ctl.!default {
    type hw
    card 0
}
```

---

## Dépannage

### Erreur: "JUCE not found"
```bash
cd ~/ComposeSiren-fixed
git submodule update --init --recursive
```

### Erreur: "X11 headers not found"
```bash
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
```

### Erreur de compilation: "Out of memory"
```bash
# Utiliser moins de cœurs
cmake --build . --config Release -j2

# Ou augmenter le swap (attention, lent)
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile  # Changer CONF_SWAPSIZE=2048
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

---

## Vérification des contrôles MIDI

Une fois compilé et lancé, testez :

**Pan (CC10)** sur canaux 1-7 :
```
Canal 1-7, CC10 = 0   → Pan à gauche
Canal 1-7, CC10 = 64  → Pan au centre
Canal 1-7, CC10 = 127 → Pan à droite
```

**Reverb (canal 16)** :
```
Canal 16, CC64 = 127  → Activer reverb
Canal 16, CC65 = 64   → Room Size 50%
Canal 16, CC66 = 64   → Dry/Wet 50%
Canal 16, CC67 = 64   → Damping 50%
Canal 16, CC68 = 64   → Highpass ~1kHz
Canal 16, CC69 = 64   → Lowpass ~11kHz
Canal 16, CC70 = 64   → Width 50%
```

---

**Dernière mise à jour:** 21 Octobre 2025  
**Corrections:** Pan + Reverb + Sync UI MIDI

