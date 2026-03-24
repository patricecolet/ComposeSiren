# re-enable e.g. if we need ProJucer support
# configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

# ------------------------------------------------------------------------------
# Shared build options (features and definitions)

add_library(ComposeSirenBuildOptions INTERFACE)

# without this, build uses c++14 somehow,
# disregarding other settings about CXX standard.
# this should be fixed since set(CMAKE_CXX_STANDARD_REQUIRED ON)
# in root CMakeLists.txt was missing "ON"
# target_compile_features(ComposeSirenBuildOptions INTERFACE cxx_std_20)

# normalize resources path across OSes and use it in Debug mode
file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}/Resources" DEV_RES_DIR)

target_compile_definitions(ComposeSirenBuildOptions
  INTERFACE
    CMS_BUILD_WITH_CMAKE=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    # build version info
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_DESCRIPTION=${PROJECT_DESCRIPTION}
    GIT_HASH=${GIT_HASH}
    # set symbols according to build type
    # works for single and multi configs
    $<$<CONFIG:Debug>:COMPOSESIREN_DEV_BUILD=1>
    $<$<NOT:$<CONFIG:Debug>>:COMPOSESIREN_DEV_BUILD=0>
    $<$<CONFIG:Debug>:COMPOSESIREN_DEV_RESOURCES_DIR=${DEV_RES_DIR}>
    $<$<NOT:$<CONFIG:Debug>>:COMPOSESIREN_DEV_RESOURCES_DIR="">
)

if(LINUX)
  # to fix linker error looking for curl symbols
  target_compile_definitions(ComposeSirenBuildOptions
    INTERFACE JUCE_USE_CURL=0
  )
endif()

# ------------------------------------------------------------------------------
# Add shared core library

add_subdirectory(ComposeSirenCore)
target_link_libraries(ComposeSirenCore PUBLIC ComposeSirenBuildOptions)
target_link_libraries(ComposeSirenCore PUBLIC ${BaseTargetName}BinaryData)

# ------------------------------------------------------------------------------
# Add plugin variants

function(add_plugin_variant TARGET_NAME PLUGIN_CODE BUNDLE_ID)
  juce_add_plugin(${TARGET_NAME}
    PRODUCT_NAME "${TARGET_NAME}"
    COMPANY_NAME "${VendorName}"
    VERSION ${CMAKE_PROJECT_VERSION}
    ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/Icon.png"
    ICON_SMALL "${CMAKE_SOURCE_DIR}/Assets/Icon.png"
    FORMATS ${FORMATS}

    # A four-character manufacturer id with at least one upper-case character
    PLUGIN_MANUFACTURER_CODE ${PLUGIN_MANUFACTURER_CODE}
    # A unique four-character plugin id with at least one upper-case character
    PLUGIN_CODE ${PLUGIN_CODE}
    # the reverse url style unique identifiers (com.<vendor>.<plugin_name>)
    BUNDLE_ID "${BUNDLE_ID}"

    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE

    PLUGIN_NAME "${TARGET_NAME}"
  )

  juce_generate_juce_header(${TARGET_NAME})

  target_sources(${TARGET_NAME}
    PRIVATE
      "Plugins/${TARGET_NAME}/PluginProcessor.cpp"
      "Plugins/${TARGET_NAME}/PluginEditor.cpp"
  )

  target_link_libraries(${TARGET_NAME}
    PRIVATE
      ComposeSirenBuildOptions
      ComposeSirenCore
      # Binary data defined in Assets directory (only needed by Core)
      # ${BaseTargetName}BinaryData
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

  # add if really needed :
  # target_compile_definitions(${TARGET_NAME}
  #   PRIVATE
  #   SIREN_VARIANT_TARGET_NAME="${TARGET_NAME}"
  # )
endfunction()

# ------------------------------------------------------------------------------
# Add the plugin variant targets

add_plugin_variant(OneSiren       "MvOS" "com.mecaviv.onesiren")

#add_plugin_variant(SirenOrchestra "MvSO" "com.mecaviv.sirenorchestra")

#add_plugin_variant(CustomMix      "MvCM" "com.mecaviv.custommix")
#TARGET_SOURCES(CustomMix
#  PRIVATE
#    "Plugins/CustomMix/CS_MidiIn.cpp"
#    "Plugins/CustomMix/Synth.cpp"
#    "Plugins/CustomMix/Components/MainCommandsComponent.cpp"
#    "Plugins/CustomMix/Components/MixerComponent.cpp"
#    "Plugins/CustomMix/Components/MixerStripComponent.cpp"
#    "Plugins/CustomMix/Components/ReverbComponent.cpp"
#)