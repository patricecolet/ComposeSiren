cmake_minimum_required(VERSION 3.19.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)
project(ComposeSiren VERSION 1.5.0)
set(BaseTargetName ${PROJECT_NAME})
set(VendorName "MecaniqueVivante")

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
  PRODUCT_NAME "${BaseTargetName}"
  COMPANY_NAME "${VendorName}"
  VERSION ${CMAKE_PROJECT_VERSION}
  ICON_BIG "${CMAKE_SOURCE_DIR}/Resources/Icon.png"
  ICON_SMALL "${CMAKE_SOURCE_DIR}/Resources/Icon.png"
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
  COPY_PLUGIN_AFTER_BUILD FALSE     # Should the plugin be installed to a default location after building?
  # VST3_CATEGORIES
  # AAX_CATEGORY
  # AU_MAIN_TYPE
  # COMPANY_WEBSITE https://www.mecanique-vivante.com
  # COMPANY_EMAIL
  BUNDLE_ID "com.mecaviv.${BaseTargetName}"
  PLUGIN_NAME "${BaseTargetName}"
  PRODUCT_NAME "${BaseTargetName}"
)

juce_generate_juce_header(${BaseTargetName})

# without this, build uses c++14 somehow, disregarding other settings about CXX standard.
target_compile_features ("${BaseTargetName}" PRIVATE cxx_std_20)
target_compile_definitions("${BaseTargetName}" PRIVATE CMS_BUILD_WITH_CMAKE=1)

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

################################################################################
# packaging

# if JUCE_COPY_PLUGIN_AFTER_BUILD is set to true, means we will install non
# signed binaries locally (e.g. for debugging purpose) and we likely don't want
# to build an installer right now, so we stop here :

if(JUCE_COPY_PLUGIN_AFTER_BUILD)
  return()
endif()

if(APPLE) # we generate a cmake subproject to build the installer
  include(Installers/Installer.Generate.cmake)
elseif(WIN32) # we just proceed "normally"
  include(Installers/CPackBuildInstaller.cmake)
endif()
