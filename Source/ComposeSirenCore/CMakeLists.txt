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

  lib/definitions/sirenProperties.cpp
  lib/definitions/parameterDefinitions.cpp

  # lib/dsp/CS_midiIN.cpp
  lib/dsp/MidiIN.cpp
  lib/dsp/Sirene.cpp
  lib/dsp/allpass.cpp
  lib/dsp/comb.cpp
  lib/dsp/mareverbe.cpp

  lib/wrappers/SirenVoice.cpp
  lib/wrappers/SirenEnsemble.cpp

  Components/SirenStripComponent.cpp
  Components/BasicKnobComponent.cpp
)

# ------------------------------------------------------------------------------

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

# If we need additional global compile defs, put them here
#target_compile_definitions(ComposeSirenCore
#  PUBLIC
#    JUCE_VST3_CAN_REPLACE_VST2=0
#)