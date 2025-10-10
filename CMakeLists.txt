cmake_minimum_required(VERSION 3.19.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
# Get the git hash
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
set(PROJECT_DESCRIPTION "ComposeSiren du ${CURRENT_DATE} sur commit n ${GIT_HASH}")

project(ComposeSiren VERSION 1.5.0 DESCRIPTION ${PROJECT_DESCRIPTION})
set(BaseTargetName ${PROJECT_NAME})
set(VendorName "Mecanique Vivante")

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

juce_add_plugin(${BaseTargetName}
  PRODUCT_NAME "${BaseTargetName}"
  COMPANY_NAME "${VendorName}"
  VERSION ${CMAKE_PROJECT_VERSION}
  ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/Icon_1024.png"
  ICON_SMALL "${CMAKE_SOURCE_DIR}/Assets/Icon_1024.png"
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

add_subdirectory(Assets)
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

if(LINUX)
  # to fix linker error looking for curl symbols
  target_compile_definitions("${BaseTargetName}" PRIVATE JUCE_USE_CURL=0)

  # to fix
  # relocation R_X86_64_PC32 against symbol `_ZN10BinaryData18knobbackground_svgE' can not be used when making a shared object; recompile with -fPIC
  # https://forum.juce.com/t/linux-cmake-and-fpic/54149/2
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

target_compile_definitions(${BaseTargetName} 
  PUBLIC 
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
)

################################################################################
# packaging

# if JUCE_COPY_PLUGIN_AFTER_BUILD is set to true, it means we will install non
# signed binaries locally (e.g. for debugging purpose) and we likely don't want
# to build an installer right now, so we stop here :

if(JUCE_COPY_PLUGIN_AFTER_BUILD)
  return()
endif()

if(APPLE) # we generate a cmake subproject to build the installer
  include(Packaging/Apple/MakePackage.cmake)
elseif(WIN32) # we just proceed the regular way
  include(Packaging/Windows/MakePackage.cmake)
elseif(LINUX) # Debian package for Raspberry Pi and other Linux systems
  include(Packaging/Linux/MakePackage.cmake)
endif()
