# ComposeSiren sur Raspberry Pi

Guide complet pour compiler, déployer et utiliser ComposeSiren sur Raspberry Pi.

---

## 📋 Table des matières

- [Installation rapide](#installation-rapide)
- [Compilation via Docker (Mac → ARM64)](#compilation-via-docker-mac--arm64)
- [Prérequis système](#prérequis-système)
- [Compilation](#compilation)
- [Déploiement automatique](#déploiement-automatique)
- [Configuration audio](#configuration-audio)
- [Tests MIDI](#tests-midi)
- [Dépannage](#dépannage)

---

## 🚀 Installation rapide

### Méthode 1 : Build Docker (Mac → Raspberry Pi, recommandé)

Compile sur ton Mac dans un conteneur Linux ARM64. Plus rapide que de compiler sur le Pi.

```bash
cd /path/to/ComposeSiren-fixed
./scripts/build_linux_arm64_docker.sh
```

Prérequis : Docker Desktop. Le script construit l’image, compile en mode headless (**Standalone** uniquement), puis écrit les artefacts dans `build/`. Tu peux copier l'exécutable et `Resources/` sur le Pi avec `scp`.

Voir [Compilation via Docker](#compilation-via-docker-mac--arm64) pour les détails.

### Méthode 2 : Script automatique sur le Pi

```bash
cd ~/ComposeSiren-fixed
bash scripts/deploy-raspberry.sh
```

Le script fait tout automatiquement :
- ✅ Installe les dépendances
- ✅ Met à jour le code (git pull)
- ✅ Compile (30-60 min)
- ✅ Crée et installe le package .deb

### Méthode 3 : Étapes manuelles

Voir la section [Compilation](#compilation) ci-dessous pour les détails.

---

## 🐳 Compilation via Docker (Mac → ARM64)

Pour éviter la longue compilation sur le Raspberry Pi (surtout `juce_gui_extra.cpp`), tu peux compiler sur ton Mac dans un conteneur Docker Linux ARM64.

**Prérequis** : Docker Desktop installé et démarré.

```bash
cd /path/to/ComposeSiren-fixed
./scripts/build_linux_arm64_docker.sh
```

Le script :
1. Construit l’image `composesiren-build:linux-arm64` (Debian Bookworm + deps JUCE).
2. Monte le repo dans le conteneur, lance `cmake` et `cmake --build` en mode **headless** (cible **Standalone** uniquement).
3. Écrit le binaire dans `build/ComposeSiren_Orchestra_artefacts/Release/Standalone/`.

Ensuite, copie l'exécutable et les ressources sur le Pi :

```bash
scp "build/.../Standalone/ComposeSiren Orchestra" user@raspberry:~/
scp -r Resources user@raspberry:~/ComposeSiren_Resources
```

Sur le Pi, lance le Standalone en headless (sans X11) : `./"ComposeSiren Orchestra"`.

---

## 🔧 Prérequis système

### Raspberry Pi testé
- **Modèle** : Raspberry Pi 4 (recommandé) ou 3B+
- **OS** : Raspberry Pi OS (64-bit recommandé)
- **RAM** : 2GB minimum, 4GB recommandé
- **Carte son** : HifiBerry DAC ou compatible

### Installation des dépendances

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

## 🛠️ Compilation

### Étape 1 : Récupérer le code depuis GitHub

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

### Étape 2 : Configurer avec CMake

```bash
cd ~/ComposeSiren-fixed

# Créer le dossier de build
mkdir -p build-raspberry
cd build-raspberry

# Configurer le projet
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Étape 3 : Compiler

```bash
# Compiler (cela peut prendre 30-60 minutes sur Raspberry Pi)
cmake --build . --config Release -j2

# Note: -j2 utilise 2 cœurs en parallèle
# Si votre Raspberry chauffe trop, utilisez -j1
```

### Étape 4 : Créer le package Debian

```bash
# Créer le package .deb
cpack

# Le package sera créé ici :
# build-raspberry/Packaging/ComposeSiren_Installer_artefacts/composesiren_1.5.0_arm64.deb
```

### Étape 5 : Installer le package

```bash
# Installer avec dpkg
sudo dpkg -i Packaging/ComposeSiren_Installer_artefacts/composesiren_*.deb

# Le package installe automatiquement :
# - /usr/bin/ComposeSiren (binaire)
# - /usr/share/ComposeSiren/Resources/ (données)
# - /usr/share/applications/composesiren.desktop (menu)
# - /usr/share/pixmaps/composesiren.png (icône)
```

### Test rapide

```bash
# Vérifier l'installation
which ComposeSiren
dpkg -l | grep composesiren

# Tester le lancement (Ctrl+C pour quitter)
ComposeSiren
```

---

## 🤖 Déploiement automatique

Le script `scripts/deploy-raspberry.sh` automatise tout le processus.

### Utilisation

```bash
cd ~/ComposeSiren-fixed
bash scripts/deploy-raspberry.sh
```

### Ce que fait le script

1. **Vérifie et installe les dépendances** système
2. **Met à jour le code** depuis GitHub (`git pull`)
3. **Configure** le build avec CMake
4. **Compile** le projet (avec -j2 pour éviter la surchauffe)
5. **Crée le package** .deb
6. **Installe** automatiquement le package
7. **Vérifie** l'installation

### Transfert depuis le Mac

Si vous développez sur Mac et voulez déployer sur le Raspberry Pi :

```bash
# Depuis votre Mac
cd /Users/patricecolet/repo/ComposeSiren-fixed

# Pousser vos modifications sur GitHub
git push origin master

# SSH vers le Raspberry
ssh sirenateur@<IP_RASPBERRY>

# Sur le Raspberry, exécuter le script de déploiement
cd ~/ComposeSiren-fixed
bash scripts/deploy-raspberry.sh
```

---

## 🔊 Configuration audio (HifiBerry DAC)

Le script `start-raspberry.sh` gère déjà PulseAudio, mais vérifiez que la HifiBerry est détectée :

### Vérification de la carte son

```bash
# Lister les cartes son
aplay -l

# Tester la sortie
speaker-test -c 2 -t wav -D hw:0,0
```

### Configuration ALSA (si nécessaire)

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

### Vérification HifiBerry

```bash
# Vérifier que HifiBerry est détectée
cat /proc/asound/cards

# Devrait afficher quelque chose comme :
# 0 [sndrpihifiberry]: HifiBerry-DAC - snd_rpi_hifiberry_dac
```

---

## 🎹 Tests MIDI

Une fois ComposeSiren installé et lancé, testez les contrôles MIDI.

### Pan (CC10) - Canaux 1-7

```
Canal 1-7, CC10 = 0   → Pan à gauche
Canal 1-7, CC10 = 64  → Pan au centre
Canal 1-7, CC10 = 127 → Pan à droite
```

**Exemple** : Pour déplacer la sirène S1 à gauche :
- Envoyez CC10 avec valeur 0 sur le canal MIDI 1

### Reverb - Canal 16

| CC  | Paramètre | Description |
|-----|-----------|-------------|
| 64  | Enable    | Activer reverb (valeur ≥64) |
| 65  | Room Size | Taille de la réverbération (0-127) |
| 66  | Dry/Wet   | Balance signal sec/effet (0=100% dry, 127=100% wet) |
| 67  | Damping   | Amortissement hautes fréquences (0-127) |
| 68  | Highpass  | Filtre passe-haut (0-127 → 20Hz-2kHz) |
| 69  | Lowpass   | Filtre passe-bas (0-127 → 2kHz-20kHz) |
| 70  | Width     | Largeur stéréo (0=mono, 127=stéréo large) |

**Exemple** : Pour activer la reverb avec taille moyenne :
```
Canal 16, CC64 = 127  → Activer reverb
Canal 16, CC65 = 64   → Room Size 50%
Canal 16, CC66 = 64   → Dry/Wet 50%
Canal 16, CC67 = 64   → Damping 50%
Canal 16, CC68 = 64   → Highpass ~1kHz
Canal 16, CC69 = 64   → Lowpass ~11kHz
Canal 16, CC70 = 64   → Width 50%
```

### Reset

- **CC121 sur canaux 1-7** : Réinitialise le canal spécifique
- **CC121 sur canal 16** : Réinitialise TOUS les canaux (1-7)

**L'interface doit maintenant réagir en temps réel aux changements MIDI !** 🎹✨

---

## 🐛 Dépannage

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
cmake --build . --config Release -j1

# Ou augmenter le swap (attention, lent)
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile  # Changer CONF_SWAPSIZE=2048
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

### ComposeSiren ne démarre pas

```bash
# Vérifier les logs
journalctl -xe | grep ComposeSiren

# Vérifier les dépendances
ldd /usr/bin/ComposeSiren

# Tester en verbose
ComposeSiren --verbose
```

### Audio ne sort pas

```bash
# Vérifier PulseAudio
pulseaudio --check
pulseaudio --start

# Vérifier le volume
alsamixer

# Tester la sortie audio
speaker-test -c 2 -t wav

# Vérifier les sorties JACK (si utilisé)
jack_lsp
```

### MIDI non détecté

```bash
# Lister les ports MIDI
aconnect -l

# Vérifier les connexions MIDI
aseqdump

# Reconnecter le contrôleur MIDI
sudo systemctl restart alsa-state
```

---

## 🔄 Mise à jour

Pour mettre à jour ComposeSiren vers une nouvelle version :

```bash
cd ~/ComposeSiren-fixed

# Méthode automatique (recommandée)
bash scripts/deploy-raspberry.sh

# OU Méthode manuelle :
git pull origin master
git submodule update --init --recursive
cd build-raspberry
cmake --build . --config Release -j2
cpack
sudo dpkg -i Packaging/ComposeSiren_Installer_artefacts/composesiren_*.deb
```

---

## 🗑️ Désinstallation

```bash
# Désinstaller le package
sudo dpkg -r composesiren

# Supprimer les fichiers de configuration (optionnel)
rm -rf ~/.config/ComposeSiren
```

---

## 📝 Notes de version

### v1.5.0 (Octobre 2025)
✅ **Pan (CC10)** fonctionne sur canaux 1-7  
✅ **Reverb (CC64-70)** fonctionne sur canal 16  
✅ **Synchronisation MIDI → Interface** en temps réel  
✅ Mixer 7 canaux avec Volume + Pan individuels  
✅ Reset MIDI via CC121  
✅ Sauvegarde d'état automatique  

---

## 📞 Support

- **Documentation principale** : [README.md](../README.md)
- **Contrôles MIDI détaillés** : [MIDI_CONTROL.md](MIDI_CONTROL.md)
- **Site Mécanique Vivante** : https://www.mecanique-vivante.com

---

**Dernière mise à jour** : 21 Octobre 2025

