add_library(ComposeSirenCore STATIC)

set_target_properties(ComposeSirenCore PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# ------------------------------------------------------------------------------
# config.h generation : keep metadata/config here if still in use

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/../config.h.in"
  "${GENERATED_DIR}/config.h"
  @ONLY
)

target_include_directories(ComposeSirenCore
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}"
    "${GENERATED_DIR}"
)

# juce_generate_juce_header(ComposeSirenCore)

# ------------------------------------------------------------------------------

target_sources(ComposeSirenCore
  PRIVATE
  # Synth.cpp
  Reverb.cpp

  lib/definitions/sirenProperties.cpp
  lib/definitions/parameterDefinitions.cpp

  # lib/dsp/CS_midiIN.cpp
  lib/dsp/MidiIn.cpp
  lib/dsp/Sirene.cpp
  lib/dsp/allpass.cpp
  lib/dsp/comb.cpp
  lib/dsp/mareverbe.cpp

  lib/wrappers/SirenVoice.cpp
  lib/wrappers/SirenEnsemble.cpp

  lib/net/mcp/McpControl.cpp
  lib/net/mcp/McpDeviceHooks.cpp

  Components/SirenStripComponent.cpp
  Components/SirenTrackComponent.cpp
  Components/ReverbStripComponent.cpp
  Components/MasterVolumeComponent.cpp
)

# ------------------------------------------------------------------------------

if(COMPOSESIREN_MCP)
  include("${CMAKE_CURRENT_LIST_DIR}/Mcp.cmake")
  target_link_libraries(ComposeSirenCore PUBLIC composesiren_mcp)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_MCP=1)
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_MCP=0)
endif()

if(COMPOSESIREN_CLIC)
  include("${CMAKE_CURRENT_LIST_DIR}/Clic.cmake")
  target_link_libraries(ComposeSirenCore PUBLIC clic_composesiren)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_CLIC=1)
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_CLIC=0)
endif()

if(COMPOSESIREN_DSP_CONTROL_MIDI_IN)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_DSP_CONTROL_MIDI_IN=1)
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_DSP_CONTROL_MIDI_IN=0)
endif()

if(COMPOSESIREN_SIREN_CONTROL_MIDI_IN)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_SIREN_CONTROL_MIDI_IN=1)
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_SIREN_CONTROL_MIDI_IN=0)
endif()

if(COMPOSESIREN_RECORD)
  include("${CMAKE_CURRENT_LIST_DIR}/Record.cmake")
  target_link_libraries(ComposeSirenCore PUBLIC composesiren_record)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_RECORD=1)
  target_sources(ComposeSirenCore PRIVATE
    lib/utilities/recorder/Recorder.cpp
    lib/utilities/recorder/RecordDialog.cpp
  )
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_RECORD=0)
endif()

if(COMPOSESIREN_RUST_DSP)
  include("${CMAKE_CURRENT_LIST_DIR}/RustDsp.cmake")
  target_link_libraries(ComposeSirenCore PUBLIC siren_dsp reverb_dsp)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_RUST_DSP=1)
  target_sources(ComposeSirenCore PRIVATE lib/wrappers/DspChoice.cpp)
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_RUST_DSP=0)
endif()

# Settings metadata (committed) + power-user Settings and the Settings... window: in
# every build (each option gates only its own rows, Settings::isAvailable).
target_sources(ComposeSirenCore PRIVATE lib/settings/Settings.cpp lib/settings/SettingsDialog.cpp)

if(COMPOSESIREN_SONG_TITLE)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_SONG_TITLE=1)
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_SONG_TITLE=0)
endif()

# Controller reset: keep the DSP and plugin declarations in sync. The MCP
# tool is enabled by the matching Rust feature in Mcp.cmake.
target_compile_definitions(ComposeSirenCore PUBLIC
  COMPOSESIREN_RESETALLCONTROLLERS=$<BOOL:${COMPOSESIREN_RESETALLCONTROLLERS}>
)

target_link_libraries(ComposeSirenCore
  PRIVATE
    # copied from projucer project
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    # recommended
    juce::juce_recommended_warning_flags
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)

# ------------------------------------------------------------------------------
# Generated metadata, committed: the settings and the waves need it. Generated
# from mecaviv-dev-root/resources/metadata; mecaviv members regenerate it with
# `franz doctor fix meta`, and `franz doctor meta` tells when it drifts.

foreach(_generated SirenMetadata.h SettingsMetadata.h)
  if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/definitions/generated/${_generated}")
    message(FATAL_ERROR
      "Source/ComposeSirenCore/lib/definitions/generated/${_generated} is missing. "
      "It is committed: restore it from git (mecaviv members can also regenerate it "
      "with `franz doctor fix meta`).")
  endif()
endforeach()

if(COMPOSESIREN_SIREN_WAVES)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_SIREN_WAVES=1)
  target_sources(ComposeSirenCore PRIVATE Components/waves/SirenWaveColumn.cpp)
  target_link_libraries(ComposeSirenCore PRIVATE juce::juce_opengl)
  # Debug builds read the shader from here and reload it when it is saved.
  # PUBLIC: SirenWaveColumn.h has members under this define, so every target
  # that includes it (the plugin editors) must see it too, or the class has
  # two layouts and the core code writes past the editor's member.
  target_compile_definitions(ComposeSirenCore PUBLIC
    $<$<CONFIG:Debug>:COMPOSESIREN_WAVES_SHADER_DIR=${CMAKE_SOURCE_DIR}/Assets/shaders>)
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_SIREN_WAVES=0)
endif()

# ------------------------------------------------------------------------------
# Park bridge: compiled only when COMPOSESIREN_PARK_BRIDGE is ON. Then
# SirenLink, or the mecaviv-bridge Rust library (see root CMakeLists.txt).
# SirenUdpBridge.h picks the matching implementation from
# COMPOSESIREN_MECAVIV_BRIDGE.

if(COMPOSESIREN_PARK_BRIDGE)
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_PARK_BRIDGE=1)
  if(COMPOSESIREN_MECAVIV_BRIDGE)
    include("${CMAKE_CURRENT_LIST_DIR}/MecavivBridge.cmake")
    target_link_libraries(ComposeSirenCore PUBLIC mecaviv_bridge_composesiren)
  else()
    target_sources(ComposeSirenCore PRIVATE lib/net/SirenLink.cpp)
    if(WIN32)
      # sockets de lib/net/SirenLink
      target_link_libraries(ComposeSirenCore PRIVATE ws2_32)
    endif()
  endif()
else()
  target_compile_definitions(ComposeSirenCore PUBLIC COMPOSESIREN_PARK_BRIDGE=0)
endif()

# Controller reset characterization tests: with -DBUILD_TESTING=ON, when the option is on.
if(BUILD_TESTING AND COMPOSESIREN_RESETALLCONTROLLERS)
  add_executable(ComposeSirenControllerResetTests tests/ControllerResetTests.cpp)
  target_link_libraries(ComposeSirenControllerResetTests PRIVATE
    ComposeSirenCore juce::juce_audio_processors)
  add_test(NAME controller-reset COMMAND ComposeSirenControllerResetTests)
endif()

# The Settings... window, in every build.
if(BUILD_TESTING)
  add_executable(ComposeSirenSettingsDialogTests tests/SettingsDialogTests.cpp)
  target_link_libraries(ComposeSirenSettingsDialogTests PRIVATE
    ComposeSirenCore juce::juce_audio_processors)
  add_test(NAME settings-dialog COMMAND ComposeSirenSettingsDialogTests)
endif()

# A reloaded plugin: bridges resend restored values, the reverb switch shows them, and
# controllers sent while a siren loads reach it.
if(BUILD_TESTING)
  add_executable(ComposeSirenStateRestoreTests tests/StateRestoreTests.cpp)
  target_link_libraries(ComposeSirenStateRestoreTests PRIVATE
    ComposeSirenCore juce::juce_audio_processors)
  add_test(NAME state-restore
    COMMAND ComposeSirenStateRestoreTests "${CMAKE_SOURCE_DIR}/Resources/")
endif()

# The Rust synthesis plays as the C++ one, bit for bit, in the plugin's voice.
if(BUILD_TESTING AND COMPOSESIREN_RUST_DSP)
  add_executable(ComposeSirenRustDspParityTests tests/RustDspParityTests.cpp)
  target_link_libraries(ComposeSirenRustDspParityTests PRIVATE
    ComposeSirenCore juce::juce_audio_processors)
  add_test(NAME rust-dsp-parity
    COMMAND ComposeSirenRustDspParityTests "${CMAKE_SOURCE_DIR}/Resources/")
  add_executable(ComposeSirenDspHotSwapTests tests/DspHotSwapTests.cpp)
  target_link_libraries(ComposeSirenDspHotSwapTests PRIVATE
    ComposeSirenCore juce::juce_audio_processors)
  add_test(NAME dsp-hot-swap
    COMMAND ComposeSirenDspHotSwapTests "${CMAKE_SOURCE_DIR}/Resources/")
endif()

# If we need additional global compile defs, put them here
#target_compile_definitions(ComposeSirenCore
#  PUBLIC
#    JUCE_VST3_CAN_REPLACE_VST2=0
#)
