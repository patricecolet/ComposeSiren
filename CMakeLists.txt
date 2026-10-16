cmake_minimum_required(VERSION 3.22.0)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/BuildInfo.cmake") # composesiren_option, build info
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
  # Host architecture by default. Fat x86_64+arm64 binaries: -DMACOS_UNIVERSAL=ON
  composesiren_option(MACOS_UNIVERSAL
          "Build a universal (x86_64 and arm64) macOS binary"
          OFF)
  if(MACOS_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64"
            CACHE STRING "macOS architectures" FORCE)
  else()
    # uname: CMAKE_HOST_SYSTEM_PROCESSOR is not set until project(), and
    # CMAKE_OSX_ARCHITECTURES has to be set before that. A leftover x86_64
    # in the cache (CLion, or the previous universal default) is overwritten.
    execute_process(
      COMMAND uname -m
      OUTPUT_VARIABLE _macos_uname
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_macos_uname MATCHES "^(arm64|aarch64)$")
      set(_macos_host_arch arm64)
    else()
      set(_macos_host_arch x86_64)
    endif()
    set(CMAKE_OSX_ARCHITECTURES "${_macos_host_arch}"
            CACHE STRING "macOS architectures" FORCE)
  endif()
  list(LENGTH CMAKE_OSX_ARCHITECTURES _macos_arch_count)
  if(_macos_arch_count GREATER 1)
    set(MACOS_PKG_ARCH "universal")
  else()
    set(MACOS_PKG_ARCH "${CMAKE_OSX_ARCHITECTURES}")
  endif()
  message(STATUS "macOS architectures: ${CMAKE_OSX_ARCHITECTURES}")
  # see : https://forum.juce.com/t/best-way-to-build-with-support-for-older-macos-versions/64879/3
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "minimum supported target macos version")
endif()
# in windows, specifying target archs can be done by creating dedicated CMake profiles named
# "Debug/Release amd64/arm64" and associating them the corresponding visual studio toolchains

if (LINUX)
  # to fix :
  #   relocation R_X86_64_PC32 against symbol
  #   `_ZN10BinaryData18knobbackground_svgE'
  #   can not be used when making a shared object; recompile with -fPIC
  #   https://forum.juce.com/t/linux-cmake-and-fpic/54149/2
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Get the git hash
set(GIT_HASH "Unknown")
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _git_hash_out
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _git_hash_rc
)

if(_git_hash_rc EQUAL 0 AND NOT "${_git_hash_out}" STREQUAL "")
  set(GIT_HASH "${_git_hash_out}")
endif()

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
set(PROJECT_DESCRIPTION "ComposeSiren du ${CURRENT_DATE} sur commit ${GIT_HASH}")

project(ComposeSiren VERSION 2.0.0 DESCRIPTION "${PROJECT_DESCRIPTION}")
set(PROJECT_VERSION_SUFFIX "-beta1") # just for building/packaging purpose
set(BaseTargetName ${PROJECT_NAME})
set(VendorName "Mecanique Vivante")
set(PLUGIN_MANUFACTURER_CODE "McVv")

# DEAL WITH PLUGIN RESOURCES PROCESSING ########################################

composesiren_option(PROCESS_RESOURCES
        "Use ResourcesProcessing outputs instead of original Resources"
        ON)

set(ORIGINAL_RESOURCES_DIR
        "${CMAKE_SOURCE_DIR}/Resources"
        CACHE PATH "Original resources directory")

set(PROCESSED_RESOURCES_DIR
        "${CMAKE_BINARY_DIR}/Resources-processed"
        CACHE PATH "Processed resources output directory")

if(PROCESS_RESOURCES)
  set(PLUGIN_RESOURCES_DIR
          "${PROCESSED_RESOURCES_DIR}"
          CACHE PATH "Resources directory for packaging" FORCE)
else()
  set(PLUGIN_RESOURCES_DIR
          "${ORIGINAL_RESOURCES_DIR}"
          CACHE PATH "Resources directory for packaging" FORCE)
endif()

# DEAL WITH THE PARK BRIDGE ####################################################
# COMPOSESIREN_PARK_BRIDGE compiles the UDP park bridge at all. Off: neither
# SirenLink nor the Rust crate is built, and the "Sirenes physiques" / "ST"
# controls are omitted. On: SirenOrchestra drives the physical sirens with
# SirenLink (C++, ComposeSirenCore/lib/net) or, if COMPOSESIREN_MECAVIV_BRIDGE
# is also ON, with mecaviv-bridge-composesiren (Rust). See
# Source/ComposeSirenCore/MecavivBridge.cmake.

composesiren_option(COMPOSESIREN_PARK_BRIDGE
        "Compile the park UDP bridge (C++ SirenLink or Rust mecaviv-bridge) and show Sirenes physiques / ST"
        OFF)

composesiren_option(COMPOSESIREN_MECAVIV_BRIDGE
        "Drive the park with mecaviv-bridge-composesiren (Rust) instead of SirenLink (requires COMPOSESIREN_PARK_BRIDGE)"
        OFF)

# DEAL WITH THE CLICK OUTPUT ###################################################
# SirenOrchestra gets a second stereo output bus, "Clic", played by the click
# box's engine (Rust crate Source/clic-composesiren, on ClicRaspberry/clic-core of
# firmwares-artila, checked out next to ComposeSiren). MIDI channel 10: note on
# with note > 1 and velocity > 1, even note = strong click, odd = weak click;
# program change = choice of click. See Source/ComposeSirenCore/Clic.cmake.

composesiren_option(COMPOSESIREN_CLIC
        "Click output: a Clic stereo bus on SirenOrchestra, MIDI channel 10 (Rust, firmwares-artila next to ComposeSiren)"
        OFF)

# DEAL WITH DSP CONTROL MIDI-IN ################################################
# SirenOrchestra opens an extra MIDI-in port for DSP Control that is not the
# siren MIDI implementation (MidiIn): per-siren pan/gain, master volume, and
# reverb. Virtual destination "SirenOrchestra: DSP Control"; further instances
# number the SirenOrchestra prefix. Off: pan/gain/reverb stay UI and host-
# automation only.

composesiren_option(COMPOSESIREN_DSP_CONTROL_MIDI_IN
        "Extra MIDI-in port on SirenOrchestra for DSP Control (reverb, pan, gain)"
        OFF)

# DEAL WITH SIREN CONTROL MIDI-IN ##############################################
# SirenOrchestra opens an extra MIDI-in port for Siren Control that joins the
# siren MIDI implementation (MidiIn): notes, volume, vibrato, and the other
# firmware CCs. Virtual destination "SirenOrchestra: Siren Control"; further
# instances number the SirenOrchestra prefix. Off: Siren Control stays host
# MIDI (and the on-screen keyboard) only.

composesiren_option(COMPOSESIREN_SIREN_CONTROL_MIDI_IN
        "Extra MIDI-in port on SirenOrchestra for Siren Control (notes, volume, vibrato)"
        OFF)

# DEAL WITH THE MCP SERVER #####################################################
# Each plugin instance can run an in-process MCP server (Rust crate
# Source/composesiren-mcp). It does not use mecaviv-rs. See
# Source/ComposeSirenCore/Mcp.cmake.

composesiren_option(COMPOSESIREN_MCP
        "In-process MCP server for parameters, MIDI, and standalone audio/MIDI devices"
        ON)

# DEAL WITH THE CONTROLLER RESET ###############################################
# Adds thread-safe DSP reset requests, a zero-volume callback before MIDI CC
# 121 resets the controllers, and MIDI CC 120 (All Sound Off) / 123 (All Notes Off).
# With COMPOSESIREN_MCP, also adds reset_controllers (the reset-all-controllers
# Rust feature in Source/composesiren-mcp). Off: retain the main branch's DSP
# behaviour and MCP tools. See Source/ComposeSirenCore/Mcp.cmake.
# An Enable checkbox in Settings... controls it at runtime.

option(COMPOSESIREN_RESETALLCONTROLLERS
        "Thread-safe controller reset, MIDI CC 120/123, and the MCP reset_controllers tool"
        OFF)
# Register only when enabled so the default About options also match master.
if(COMPOSESIREN_RESETALLCONTROLLERS)
  set_property(GLOBAL APPEND PROPERTY COMPOSESIREN_OPTIONS COMPOSESIREN_RESETALLCONTROLLERS)
endif()

# DEAL WITH THE RECORDER #######################################################
# Records the audio output to FLAC or WAV (Rust crate Source/composesiren-record,
# pure Rust encoders): a Record dialog in the editor, and, with COMPOSESIREN_MCP,
# the start_recording, stop_recording and recording_status tools. See
# Source/ComposeSirenCore/Record.cmake.

composesiren_option(COMPOSESIREN_RECORD
        "Record the audio output to FLAC or WAV (dialog, and MCP tools with COMPOSESIREN_MCP)"
        OFF)

# DEAL WITH THE RUST DSP #######################################################
# The Rust synthesis (Rust crate Source/siren-dsp): its bit-exact port of the C++ DSP and
# the variants, chosen with the global dsp.variant and dsp.dataset settings (the C++ DSP
# stays the default). See Source/ComposeSirenCore/RustDsp.cmake and Source/siren-dsp.

composesiren_option(COMPOSESIREN_RUST_DSP
        "The Rust synthesis and its variants, chosen in the settings (dsp.variant, dsp.dataset)"
        OFF)

# THE SETTINGS #################################################################
# Power-user settings of the plugins, in every build: stored per user, edited in the
# Settings... window (top-bar menu). What they are (type, range, default, description)
# comes from Source/ComposeSirenCore/lib/definitions/generated/SettingsMetadata.h,
# committed, generated from mecaviv-dev-root's resources/metadata/composesiren-settings.csv
# (`franz doctor fix meta`). An option gates only its own rows (the controller reset
# rows need COMPOSESIREN_RESETALLCONTROLLERS). See Source/ComposeSirenCore/lib/settings.

# DEAL WITH THE SONG TITLE BAR #################################################
# Shows the playing song in the top bar and the window title, set over MCP
# (set_song_title / set_song_progress / clear_song_title, the `song-title` Rust
# feature of Source/composesiren-mcp). Needs COMPOSESIREN_MCP for the tools;
# the C++ preprocessor is COMPOSESIREN_SONG_TITLE.

composesiren_option(COMPOSESIREN_SONG_TITLE
        "Song title bar: show the playing song (MCP set_song_title / clear_song_title)"
        OFF)

# DEAL WITH THE SIREN WAVES ####################################################
# Animated waves in SirenOrchestra's siren title cells, following each siren's
# output level and pitch, in the sirens' colours from the siren metadata
# (generated/SirenMetadata.h, committed). OpenGL (juce_opengl, Assets/shaders/
# SirenWave.frag) with a software fallback. Tuned by the waves.* settings
# (the settings). See Source/ComposeSirenCore/Components/waves.

composesiren_option(COMPOSESIREN_SIREN_WAVES
        "Animated waves in SirenOrchestra's title cells (OpenGL, software fallback)"
        OFF)

#set(PLUGIN_TARGETS OneSiren)
set(PLUGIN_TARGETS OneSiren SirenOrchestra)
#set(PLUGIN_TARGETS CustomMix)

add_subdirectory(Dependencies/JUCE)

set(FORMATS "VST3" "Standalone")

if (VST2_PATH)
  list(APPEND FORMATS "VST")
  juce_set_vst2_sdk_path(${VST2_PATH})
endif()

if (AAX_PATH)
  list(APPEND FORMATS "AAX")
  juce_set_aax_sdk_path(${AAX_PATH})
endif()

if (APPLE)
  list(APPEND FORMATS "AU")
endif()

# LV2, the Linux-native plugin format (Ardour, Carla, Qtractor…), next to VST3.
if (UNIX AND NOT APPLE)
  list(APPEND FORMATS "LV2")
endif()

composesiren_add_build_info() # after the options: it records them

if(BUILD_TESTING)
  enable_testing()
endif()

# Tests: -DBUILD_TESTING=ON (CTest's standard switch) enables them, whatever else is on. These
# always run; COMPOSESIREN_RESETALLCONTROLLERS and COMPOSESIREN_RUST_DSP each add their own test
# (Source/ComposeSirenCore/CMakeLists.txt).
if(BUILD_TESTING)
  find_program(COMPOSESIREN_DOTNET dotnet)
  if(COMPOSESIREN_DOTNET)
    add_test(NAME fix-data
      COMMAND "${COMPOSESIREN_DOTNET}" fsi tests/fix-data.fsx "${CMAKE_SOURCE_DIR}/Resources"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/ResourcesProcessing")
    add_test(NAME compact-roundtrip
      COMMAND "${COMPOSESIREN_DOTNET}" fsi tests/compact-roundtrip.fsx "${CMAKE_SOURCE_DIR}/Resources"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/ResourcesProcessing")
  endif()
  find_program(COMPOSESIREN_CARGO cargo)
  if(COMPOSESIREN_CARGO)
    foreach(crate IN ITEMS siren-data siren-dsp)
      add_test(NAME cargo-${crate}
        COMMAND "${COMPOSESIREN_CARGO}" test --release
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Source/${crate}")
    endforeach()
  endif()
endif()

add_subdirectory(Assets)
add_subdirectory(Source)

if(PROCESS_RESOURCES)
  add_subdirectory(ResourcesProcessing)
endif()

if(APPLE) # we generate a cmake subproject to build the installer
  include(Packaging/Apple/MakePackage.cmake)
elseif(WIN32) # we just proceed the regular way
  include(Packaging/Windows/MakePackage.cmake)
  #include(Packaging/Wix/MakePackage.cmake)
endif()
