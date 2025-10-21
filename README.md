# ComposeSiren

Audio plugin that synthesizes sounds of sirens made by [Mécanique Vivante][1].

<p align="center">
  <img src="Assets/Icon_1024.png" alt="ComposeSiren Icon" width="200"/>
</p>

## 🎵 About

ComposeSiren handles the seven-piece Siren Orchestra:
- **S1, S2**: Two altos
- **S3**: Bass
- **S4**: Tenor  
- **S5, S6**: Two sopranos
- **S7**: Piccolo

**Formats**: VST3, Audio Unit, Standalone  
**Tested on**: macOS, Linux (Raspberry Pi), Windows

---

## 📥 Download

**Latest Release: v1.5.0 (Custom Mix)**

| Platform | Download | Notes |
|----------|----------|-------|
| **macOS** | [ComposeSiren-v1.5.0-macOS.dmg](Releases/ComposeSiren-v1.5.0-custom-mix-macOS.dmg) | Standalone + Audio Unit (47 MB) |
| **Linux (ARM64)** | [composesiren_1.5.0_arm64.deb](Releases/v1.5.0-linux/) | Debian package for Raspberry Pi |
| **Windows** | Build from source | See [Building](#-building-from-source) section |

📝 [Release Notes v1.5.0](Releases/RELEASE_NOTES_v1.5.0.md)

---

## ✨ Features (v1.5.0)

### 🎚️ Mixer (7 channels)

Each siren (S1-S7) has:
- **Volume** slider (blue for Master, grey for others)
- **Pan** knob (L/R panoramic)
- **Reset** button (reset all channel parameters)
- **MIDI Note On** indicator (green when active)

### 🌀 Reverb (Global)

Stereo reverb with controls:
- **Room Size**: Reverb size
- **Damping**: High-frequency damping
- **Dry/Wet**: Balance between dry signal and effect
- **Width**: Stereo width

### 🎹 MIDI Control

#### Channels 1-7 (Sirens S1-S7)

| CC  | Parameter     | Range | Description |
|-----|---------------|-------|-------------|
| 7   | Volume        | 0-127 | Individual siren volume |
| 10  | Pan           | 0-127 | Panoramic (0=left, 64=center, 127=right) |
| 70  | Master Volume | 0-127 | Master volume independent of CC7 |
| 121 | Reset         | any   | Reset all channel parameters |

#### Channel 16 (Global Reverb)

| CC  | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 64  | Enable    | 0-127 | Enable/disable reverb (≥64 = ON) |
| 65  | Room Size | 0-127 | Reverb size |
| 66  | Dry/Wet   | 0-127 | Balance dry/effect (0=100% dry, 127=100% wet) |
| 67  | Damping   | 0-127 | High-frequency damping |
| 68  | Highpass  | 0-127 | Highpass filter (20Hz - 2kHz) |
| 69  | Lowpass   | 0-127 | Lowpass filter (2kHz - 20kHz) |
| 70  | Width     | 0-127 | Stereo width (0=mono, 127=wide stereo) |
| 121 | Reset All | any   | Reset ALL sirens (channels 1-7) |

### 💾 State Saving

All mixer and reverb parameters are automatically saved in your DAW preset/project.

---

## 🔧 Building from Source

### Prerequisites

**All platforms:**
```bash
git clone --recursive https://github.com/patricecolet/ComposeSiren.git
cd ComposeSiren
```

**Linux dependencies:**
```bash
sudo apt-get install -y \
    cmake build-essential \
    libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libfreetype-dev libasound2-dev \
    libgl1-mesa-dev libglu1-mesa-dev
```

### macOS (Xcode - Recommended)

⚠️ **Note**: CMake build currently fails on macOS 15 due to deprecated JUCE APIs. Use Xcode instead:

```bash
# Open Xcode project
open Builds/MacOSX/ComposeSiren.xcodeproj

# Build targets:
# - ComposeSiren - Standalone Plugin
# - ComposeSiren - AU
# (VST3 currently fails on macOS 15 - waiting for JUCE fix)
```

📖 **For signed/notarized builds**: See [docs/CODESIGNING.md](docs/CODESIGNING.md)

### Linux / Raspberry Pi

```bash
# Quick method: automated script
bash scripts/deploy-raspberry.sh

# Or manual method:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j2
cd build && cpack
sudo dpkg -i Packaging/ComposeSiren_Installer_artefacts/composesiren_*.deb
```

📖 **Detailed guide**: See [docs/RASPBERRY_PI.md](docs/RASPBERRY_PI.md)

### Windows

```bash
# Configure with Visual Studio
cmake -B build -G "Visual Studio 17 2022" -C Config.cmake

# Build
cmake --build build --config Release

# Create installer
cpack --config build/CPackConfig.cmake
```

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [RASPBERRY_PI.md](docs/RASPBERRY_PI.md) | Complete guide for Raspberry Pi compilation and deployment |
| [CODESIGNING.md](docs/CODESIGNING.md) | macOS code signing and notarization (for distribution) |
| [MIDI_CONTROL.md](docs/MIDI_CONTROL.md) | Complete MIDI CC mapping reference with examples |
| [DEVELOPMENT.md](docs/DEVELOPMENT.md) | Architecture, sample rate handling, internal algorithms |

---

## 🐛 Known Issues

- **macOS 15**: CMake build fails due to deprecated JUCE APIs (`CGWindowListCreateImage`, `CVDisplayLink*`). Use Xcode instead.
- **VST3 on macOS 15**: Also fails due to same JUCE bug. Use AU or Standalone.
- Waiting for JUCE upstream fix.

---

## 📜 Version History

- **1.5.0** (Oct 2025) - Mixer + Reverb integration
  - 7-channel mixer with Volume + Pan controls
  - Global stereo reverb
  - Modern UI (dark grey background, horizontal sliders)
  - Complete MIDI CC control (channels 1-7 for sirens, channel 16 for reverb)
  - MIDI reset via CC121
  - Real-time MIDI Note On/Off indicators
  - State saving (all mixer + reverb parameters)
  - Dynamic sample rate handling (44.1kHz, 48kHz, 96kHz, etc.)

- **1.3.0** - Default panning and volume adjustments
- **1.2.0** - Audio Unit format added
- **1.1.0** - Improved GUI
- **1.0.0** - First VST3 version

---

## 🤝 Contributing

ComposeSiren is developed with the [JUCE framework](http://www.juce.com).

### Git Tips

```bash
# Clone with JUCE submodule
git clone --recursive https://github.com/patricecolet/ComposeSiren.git

# Or if already cloned without --recursive:
git submodule update --init --recursive

# If JUCE submodule gets corrupted:
git submodule deinit -f .
git submodule update --init
```

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for detailed development guidelines.

---

## 📄 License

See [Packaging/License.txt](Packaging/License.txt)

---

## 🔗 Links

- [Mécanique Vivante - The Musical Siren][1]
- [JUCE Framework](http://www.juce.com)
- [Ableton Live Plugin Guide][3]

[1]: https://www.mecanique-vivante.com/en/the-song-of-the-sirens/the-musical-siren
[3]: https://help.ableton.com/hc/en-us/sections/202295165-Plug-Ins
[4]: https://www.ableton.com/en/live/
