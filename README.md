# ComposeSiren

ComposeSiren is an audio plugin that synthesize sounds of sirens made by [Mécanique Vivante][1].

The plugin allows to handle the seven-piece Siren Orchestra:
- two altos (called S1 and S2),
- a bass (called S3),
- a tenor (called S4),
- two sopranos (called S5 and S6),
- and a piccolo (called S7).

It is currently available on two formats: VST3 and Audio Unit.


### Development

ComposeSiren is developed on top of the **JUCE** frameworks. You can find more infos about it there: http://www.juce.com.

ComposeSiren is built on macOS as a 64-bit VST3 and Audio Unit formats, and currently tested on [Ableton Live][4].


### Using the plugin

The vst3 plugin file is called **ComposeSiren.vst3**, and the Audio Unit plugin file called **ComposeSiren.component**.
You can download the vst3 plugin as well as the Audio Unit plugin directly from the **Releases** menu (to the right of the list of files).
You'll find [here][3] more info on how to use the plugins with Ableton Live.


### Mixer & Reverb (v1.5.0)

#### Interface graphique

L'interface comprend :
- **7 canaux mixer** (S1-S7) avec pour chacun :
  - Slider de **Volume** (bleu pour Master, gris pour les autres)
  - Bouton rotatif de **Pan** (panoramique L/R)
  - Bouton **Reset** (réinitialise tous les paramètres du canal)
  - Indicateur **MIDI Note On** (vert = note active)
- **Section Reverb** (Canal 16) :
  - **Room Size** : Taille de la réverbération
  - **Damping** : Amortissement des hautes fréquences
  - **Dry/Wet** : Balance signal sec/effet
  - **Width** : Largeur stéréo de la reverb

#### Contrôle MIDI

##### Canaux 1-7 (Sirènes S1-S7)

| CC  | Paramètre | Plage | Description |
|-----|-----------|-------|-------------|
| 7   | Volume    | 0-127 | Volume individuel de la sirène |
| 10  | Pan       | 0-127 | Panoramique (0=gauche, 64=centre, 127=droite) |
| 70  | Master Volume | 0-127 | Volume master indépendant du CC7 |
| 121 | Reset     | -     | Réinitialise tous les paramètres du canal |

##### Canal 16 (Reverb globale)

| CC  | Paramètre | Plage | Description |
|-----|-----------|-------|-------------|
| 64  | Enable    | 0-127 | Active/désactive la reverb (≥64 = ON) |
| 65  | Room Size | 0-127 | Taille de la réverbération |
| 66  | Dry/Wet   | 0-127 | Balance signal sec/effet |
| 67  | Damping   | 0-127 | Amortissement des hautes fréquences |
| 68  | Width     | 0-127 | Largeur stéréo (0=mono, 127=stéréo large) |
| 121 | Reset All | -     | Réinitialise TOUTES les sirènes (canaux 1-7) |

#### Sauvegarde d'état

Tous les paramètres du mixer et de la reverb sont sauvegardés automatiquement dans l'état du plugin (DAW preset/project). Cela inclut :
- Volumes et pans de chaque canal
- Tous les paramètres de la reverb
- État ON/OFF de la reverb


### Installation v1.5.0

**Téléchargement :**
- macOS : `Releases/ComposeSiren-v1.5.0-custom-mix-macOS.dmg` (47 MB)
  - Inclus : Standalone + Audio Unit
  - Note : VST3 non disponible (bug JUCE avec macOS 15, en attente d'un fix)

**Instructions d'installation** : Voir `INSTALLATION.txt` dans le DMG


### Version history:

- 1.5.0 - **Mixer + Reverb intégré** (branche custom-mix)
  - Mixer 7 canaux avec contrôles Volume + Pan individuels
  - Reverb globale avec Room Size, Damping, Dry/Wet et Width
  - Interface graphique moderne (fond gris foncé, sliders horizontaux)
  - Contrôle MIDI complet via CC (canaux 1-7 pour sirènes, canal 16 pour reverb)
  - Reset MIDI via CC121 (par canal ou global)
  - Indicateurs d'activité MIDI Note On/Off temps réel
  - Sauvegarde de l'état (tous les paramètres mixer + reverb)
- 1.3.0 - Change default panning and volume
- 1.2.0 - Audio Unit format added
- 1.1.0 - Improved GUI
- 1.0.0 - First version of the vst3 plugin


[1]: https://www.mecanique-vivante.com/en/the-song-of-the-sirens/the-musical-siren
[2]: https://minhaskamal.github.io/DownGit/#/home?url=https://github.com/patriceguyot/ComposeSiren/tree/master/Builds/MacOSX/ComposeSiren.vst3
[3]: https://help.ableton.com/hc/en-us/sections/202295165-Plug-Ins
[4]: https://www.ableton.com/en/live/
[5]: https://minhaskamal.github.io/DownGit/#/home?url=https://github.com/patriceguyot/ComposeSiren/tree/master/Builds/MacOSX/ComposeSiren.component


### Building with CMake

#### dependencies

linux:
* `sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev`

##### git tips

* first clone the repository with the `--recursive` option to fetch JUCE
  submodule, or run `git submodule update --init` after cloning.
* if at some point the `Dependencies/JUCE` submodule is altered by some IDE, you
  can reset it using `git submodule deinit -f .` then `git submodule update --init`

⚠️ **Note importante** : Le build CMake échoue actuellement sur macOS 15 à cause d'APIs obsolètes dans JUCE (`CGWindowListCreateImage`, `CVDisplayLink*`). En attendant un correctif JUCE, utilisez **Xcode** pour compiler :

At the moment the plugin is built :

* on Mac OS 11.6.4+ using **Xcode** (CMake ne fonctionne pas avec macOS 15)
  * Ouvrir `Builds/MacOSX/ComposeSiren.xcodeproj`
  * Compiler les targets : `ComposeSiren - Standalone Plugin` et `ComposeSiren - AU`
  * Note : VST3 échoue aussi à cause du même bug JUCE
* on Windows 10 using Visual Studio (couldn't get Ninja to work on windows yet)
  * `cmake -B build -G "Visual Studio 17 2022" -C Config.cmake`
  * `cmake --build build --config Release`
  * `cpack --config build/CPackConfig.cmake`
* on Linux
  * `cmake -B builds/linux -G "Unix Makefiles"`
  * `cmake --build builds/linux --config Release`
  * no instruction for installer for now
  
The resulting installer (built with `productbuild` on mac and `NSIS` on windows)
is created in `build/Packaging/ComposeSiren_Installer_artefacts`
