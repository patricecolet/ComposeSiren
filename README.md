# ComposeSiren

ComposeSiren is a suite of audio and MIDI plugins that synthesize sounds of sirens made by [Mécanique Vivante][1]
and provide automatable parameters from DAW hosts synchronized to MIDI input and output.
The plugins allow to compose pieces for Mécanique Vivante's siren orchestra in studio by simulating them in real-time.
The DAW projects can then be reused to directly play the pieces on the orchestra during live performances by controlling
the siren with the MIDI outputs.

The Siren Orchestra is composed of seven MIDI siren instruments :
- two altos (called S1 and S2),
- a bass (called S3),
- a tenor (called S4),
- two sopranos (called S5 and S6),
- and a piccolo (called S7).

There are actually two plugins:
- **OneSiren**, a simple plugin with flexible MIDI routing parameters that can simulate any siren from the orchestra.
- **SirenOrchestra**, a plugin that simulates the whole orchestra with its original fixed MIDI routing, and provides
  additional controls such as panning and volume adjustment for each simulated siren, as well as an embedded reverberation module
  and a master volume control.

Both are currently available in two formats: VST3 and Audio Unit, and also exist as standalone applications.


### Development

The ComposeSiren suite is developed on top of the **JUCE** frameworks. You can find more infos about it there: http://www.juce.com.

The plugins are built on macOS as 64-bit VST3 and Audio Unit formats, and currently tested on [Reaper][6] and [Ableton Live][4].
Upcoming windows versions are currently in the development pipeline.


### Using the plugins

Download the latest installer for your OS from the [releases page][link] and run it. This will install the VST3 versions
of both plugins (as well as their Audio Unit versions on MacOS), and the corresponding standalone applications.
<!-- 
The vst3 plugin files are respectively called **OneSiren.vst3** and **, and the Audio Unit plugin file called **ComposeSiren.component**.
You can download the vst3 plugin as well as the Audio Unit plugin directly from the **Releases** menu (to the right of the list of files).
You'll find [here][3] more info on how to use the plugins with Ableton Live.
-->

<!--
### Version history:

- 1.5.0
- 1.3.0 - Change default panning and volume
- 1.2.0 - Audio Unit format added
- 1.1.0 - Improved GUI
- 1.0.0 - First version of the vst3 plugin
-->

[1]: https://www.mecanique-vivante.com/en/the-song-of-the-sirens/the-musical-siren
[2]: https://minhaskamal.github.io/DownGit/#/home?url=https://github.com/patriceguyot/ComposeSiren/tree/master/Builds/MacOSX/ComposeSiren.vst3
[3]: https://help.ableton.com/hc/en-us/sections/202295165-Plug-Ins
[4]: https://www.ableton.com/en/live/
[5]: https://minhaskamal.github.io/DownGit/#/home?url=https://github.com/patriceguyot/ComposeSiren/tree/master/Builds/MacOSX/ComposeSiren.component


### Building with CMake

#### dependencies

##### linux

`sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev`

##### Raspberry Pi

```sh
sudo apt install cmake libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libasound2-dev
```

Resource path is hardcoded to point to `/home/sirenateur/Documents/src/mecaviv/ComposeSiren/Resources/`, so please checkout the repository in `/home/sirenateur/Documents/src/mecaviv/`:

```shell
cd ~/Documents
mkdir src
cd src
mkdir mecaviv
cd mecaviv
git clone https://github.com/mecaviv/ComposeSiren.git
```

##### git tips

* first clone the repository with the `--recursive` option to fetch JUCE
  submodule, or run `git submodule update --init` after cloning.
* if at some point the `Dependencies/JUCE` submodule is altered by some IDE, you
  can reset it using `git submodule deinit -f .` then `git submodule update --init`

At the moment the plugin is built :

* on Mac OS 11.6.4 using Ninja (Xcode works too)
  * `cmake -B build -G Ninja -C Config.cmake -DCMAKE_BUILD_TYPE=Release` to setup the build system
  * `cmake --build build --config Release` to build the plugins and generate the installer
* on Windows 10 using Visual Studio (couldn't get Ninja to work on windows yet)
  * `cmake -B build -G "Visual Studio 17 2022" -C Config.cmake`
  * `cmake --build build --config Release`
  * `cpack --config build/CPackConfig.cmake`
* on Linux or Raspberry
  * `cmake -B builds/linux -G "Unix Makefiles"`
  * `cmake --build builds/linux --config Release`
  * no instruction for installer for now

The resulting installer (built with `productbuild` on mac and `NSIS` on windows)
is created in `build/Packaging/ComposeSiren_Installer_artefacts`

NB :
* download VS 2022 Community from [HERE](https://aka.ms/vs/17/release/vs_community.exe)
  (more dl links [HERE](https://sharethis.zip/visual_studio/))