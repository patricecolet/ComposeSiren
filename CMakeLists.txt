cmake_minimum_required(VERSION 3.19.0)
project(ComposeSiren VERSION 1.4.0)
set(BaseTargetName ${PROJECT_NAME})

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")

add_subdirectory(Dependencies/JUCE)

# for now Standalone target is not supported (todo : check juce develop branch)
# update 11/11/22 : works with pre-7.0.3 develop branch
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

juce_add_plugin(${BaseTargetName}
  VERSION "1.4.0"
  COMPANY_NAME "MecaniqueVivante"
  PRODUCT_NAME "${BaseTargetName}"
  FORMATS ${FORMATS}
  # A four-character manufacturer id with at least one upper-case character
  PLUGIN_MANUFACTURER_CODE McVv
  # A unique four-character plugin id with at least one upper-case character
  PLUGIN_CODE MvCs
  IS_SYNTH TRUE                     # Is this a synth or an effect?
  NEEDS_MIDI_INPUT TRUE             # Does the plugin need midi input?
  NEEDS_MIDI_OUTPUT TRUE            # Does the plugin need midi output?
  IS_MIDI_EFFECT FALSE              # Is this plugin a MIDI effect?
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE  # Does the editor need keyboard focus?
  COPY_PLUGIN_AFTER_BUILD TRUE      # Should the plugin be installed to a default location after building?
  # VST3_CATEGORIES
  # AAX_CATEGORY
  # AU_MAIN_TYPE
  # COMPANY_WEBSITE https://www.mecanique-vivante.com
  # COMPANY_EMAIL
  BUNDLE_ID "com.mecaviv.ComposeSiren"
  PLUGIN_NAME "${BaseTargetName}"
  PRODUCT_NAME "${BaseTargetName}"
)

juce_generate_juce_header(${BaseTargetName})

add_subdirectory(Resources)
add_subdirectory(Source)

target_link_libraries(${BaseTargetName}
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

target_compile_definitions(${BaseTargetName} 
  PUBLIC 
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
)

include(Installers/CPackBuildInstaller.cmake)
