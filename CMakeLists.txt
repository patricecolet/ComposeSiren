cmake_minimum_required(VERSION 3.19.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)

# Option pour build headless (Linux sans X11/GUI)
option(HEADLESS_BUILD "Build without GUI support (for Linux servers, Raspberry Pi headless, etc.)" OFF)

if(HEADLESS_BUILD)
  message(STATUS "=== HEADLESS BUILD MODE ===")
  message(STATUS "Le plugin compilé n'aura pas d'interface graphique (hasEditor=false)")
  message(STATUS "GTK requis pour compiler, mais pas pour exécuter le plugin")
endif()

# Forcer la compatibilité minimale macOS Catalina (10.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

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

project(ComposeSiren VERSION 2.0.0 DESCRIPTION ${PROJECT_DESCRIPTION})
set(VendorName "Mecanique Vivante")
set(BaseTargetName "ComposeSiren_Orchestra")

# Inclure Config.cmake pour les variables de configuration (PLUGIN_RESOURCES_DIR, etc.)
if(EXISTS "${CMAKE_SOURCE_DIR}/Config.cmake")
  include("${CMAKE_SOURCE_DIR}/Config.cmake")
endif()

# Inclure la configuration locale pour la signature et notarisation (macOS uniquement)
if(APPLE)
  if(EXISTS "${CMAKE_SOURCE_DIR}/LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/LocalConfig.cmake")
    message(STATUS "✓ Configuration locale chargée : signature et notarisation activées")
  else()
    message(WARNING "LocalConfig.cmake non trouvé : le build ne sera pas signé ni notarisé")
    message(WARNING "Voir SIGNATURE_ET_NOTARISATION.md pour plus d'informations")
  endif()
endif()

# Vérifier les dépendances Linux (GTK3 requis pour la compilation, même en mode headless)
if(UNIX AND NOT APPLE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 gtk+-3.0)
  if(NOT GTK3_FOUND)
    message(FATAL_ERROR 
      "\n"
      "=== ERREUR: GTK3 development headers manquants ===\n"
      "JUCE nécessite les headers GTK3 pour compiler sur Linux.\n"
      "Installez-les avec:\n"
      "  sudo apt-get install libgtk-3-dev\n"
      "\n"
      "Note: En mode HEADLESS_BUILD, GTK n'est requis que pour la\n"
      "compilation, pas pour l'exécution du plugin.\n"
    )
  else()
    message(STATUS "✓ GTK3 trouvé: ${GTK3_VERSION}")
    # Ajouter manuellement les include directories GTK (nécessaire pour JUCE)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
  endif()
  
  # Désactiver les dépendances optionnelles sur Linux (AVANT d'inclure JUCE)
  # Ces options doivent être définies avant add_subdirectory(JUCE)
  set(JUCE_USE_CURL OFF CACHE BOOL "Disable CURL dependency" FORCE)
  set(JUCE_WEB_BROWSER OFF CACHE BOOL "Disable WebKit dependency" FORCE)
  message(STATUS "✓ CURL et WebKit désactivés (non requis)")
endif()

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

################################################################################
# Bibliothèque commune partagée entre tous les plugins
################################################################################

# Configurer config.h
configure_file(Source/Common/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Source/Common/config.h)

add_library(ComposeSiren_Common STATIC
  Source/Common/CS_midiIN.cpp
  Source/Common/CS_midiIN.h
  Source/Common/Sirene.cpp
  Source/Common/Sirene.h
  Source/Common/synth.cpp
  Source/Common/synth.h
  Source/Common/allpass.cpp
  Source/Common/allpass.h
  Source/Common/comb.cpp
  Source/Common/comb.h
  Source/Common/mareverbe.cpp
  Source/Common/mareverbe.h
  Source/Common/parameters.h
  ${CMAKE_CURRENT_BINARY_DIR}/Source/Common/config.h
)

set_target_properties(ComposeSiren_Common PROPERTIES
  JUCE_GENERATED_SOURCES_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/ComposeSiren_Common_artefacts
  JUCE_PRODUCT_NAME "ComposeSiren Common"
)
_juce_write_generate_time_info(ComposeSiren_Common)
juce_generate_juce_header(ComposeSiren_Common)

target_include_directories(ComposeSiren_Common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Common
    ${CMAKE_CURRENT_BINARY_DIR}/Source/Common
    ${CMAKE_CURRENT_BINARY_DIR}/ComposeSiren_Common_artefacts
    ${CMAKE_CURRENT_BINARY_DIR}/ComposeSiren_Common_artefacts/JuceLibraryCode
)

target_link_libraries(ComposeSiren_Common
  PUBLIC
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
)
# Note: Les modules GUI sont requis pour la compilation (dépendance JUCE interne)
# En mode HEADLESS_BUILD, le plugin fonctionnera sans GUI à l'exécution

target_compile_features(ComposeSiren_Common PRIVATE cxx_std_20)
target_compile_definitions(ComposeSiren_Common PUBLIC CMS_BUILD_WITH_CMAKE=1)

if(HEADLESS_BUILD)
  target_compile_definitions(ComposeSiren_Common PUBLIC COMPOSESIREN_HEADLESS=1)
endif()

if(LINUX)
  target_compile_definitions(ComposeSiren_Common PUBLIC JUCE_USE_CURL=0)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

################################################################################
# Plugin Orchestra (7 sirènes avec mixeur complet)
################################################################################

juce_add_plugin(ComposeSiren_Orchestra
  PRODUCT_NAME "ComposeSiren Orchestra"
  COMPANY_NAME "${VendorName}"
  VERSION ${CMAKE_PROJECT_VERSION}
  ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/Icon_1024.png"
  ICON_SMALL "${CMAKE_SOURCE_DIR}/Assets/Icon_1024.png"
  FORMATS ${FORMATS}
  PLUGIN_MANUFACTURER_CODE McVv
  PLUGIN_CODE MvCs
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  BUNDLE_ID "com.mecaviv.ComposeSiren.Orchestra"
  PLUGIN_NAME "ComposeSiren Orchestra"
  PRODUCT_NAME "ComposeSiren Orchestra"
)

juce_generate_juce_header(ComposeSiren_Orchestra)

target_sources(ComposeSiren_Orchestra
  PRIVATE
    Source/Orchestra/PluginProcessor.cpp
    Source/Orchestra/PluginProcessor.h
)

# Ajouter l'éditeur GUI seulement si pas en mode headless
if(NOT HEADLESS_BUILD)
  target_sources(ComposeSiren_Orchestra
    PRIVATE
      Source/Orchestra/PluginEditor.cpp
      Source/Orchestra/PluginEditor.h
  )
endif()

target_compile_features(ComposeSiren_Orchestra PRIVATE cxx_std_20)
target_compile_definitions(ComposeSiren_Orchestra PRIVATE 
  CMS_BUILD_WITH_CMAKE=1
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_WEB_BROWSER=0
)

target_link_libraries(ComposeSiren_Orchestra
  PRIVATE
    ComposeSiren_Common
    juce::juce_recommended_warning_flags
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)

if(LINUX)
  target_compile_definitions(ComposeSiren_Orchestra PRIVATE JUCE_USE_CURL=0)
endif()

add_subdirectory(Assets)

################################################################################
# Fonction helper pour créer un plugin Solo
################################################################################

function(add_siren_solo_plugin MODEL_NAME DISPLAY_NAME PLUGIN_CODE)
    set(TARGET_NAME "ComposeSiren_${MODEL_NAME}")
    
    juce_add_plugin(${TARGET_NAME}
        PRODUCT_NAME "ComposeSiren ${DISPLAY_NAME}"
        COMPANY_NAME "${VendorName}"
        VERSION ${CMAKE_PROJECT_VERSION}
        ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/Icon_1024.png"
        ICON_SMALL "${CMAKE_SOURCE_DIR}/Assets/Icon_1024.png"
        FORMATS ${FORMATS}
        PLUGIN_MANUFACTURER_CODE McVv
        PLUGIN_CODE ${PLUGIN_CODE}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        BUNDLE_ID "com.mecaviv.ComposeSiren.${MODEL_NAME}"
        PLUGIN_NAME "ComposeSiren ${DISPLAY_NAME}"
        PRODUCT_NAME "ComposeSiren ${DISPLAY_NAME}"
    )
    
    juce_generate_juce_header(${TARGET_NAME})
    
    target_sources(${TARGET_NAME}
        PRIVATE
            Source/Solo/common/SoloPluginProcessor.cpp
            Source/Solo/common/SoloPluginProcessor.h
    )
    
    # Ajouter l'éditeur GUI seulement si pas en mode headless
    if(NOT HEADLESS_BUILD)
      target_sources(${TARGET_NAME}
          PRIVATE
              Source/Solo/common/SoloPluginEditor.cpp
              Source/Solo/common/SoloPluginEditor.h
      )
    endif()
    
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        CMS_BUILD_WITH_CMAKE=1
        SIREN_MODEL="${MODEL_NAME}"
        SIREN_DISPLAY_NAME="${DISPLAY_NAME}"
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        SOLO_PLUGIN=1
    )
    
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            ComposeSiren_Common
            juce::juce_recommended_warning_flags
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
    )
    
    if(LINUX)
        target_compile_definitions(${TARGET_NAME} PRIVATE JUCE_USE_CURL=0)
    endif()
endfunction()

################################################################################
# Plugins Solo (à décommenter au fur et à mesure de leur création)
################################################################################

add_siren_solo_plugin("S1" "S1 Alto" "Cs01")
add_siren_solo_plugin("S3" "S3 Bass" "Cs03")
add_siren_solo_plugin("S4" "S4 Tenor" "Cs04")
add_siren_solo_plugin("S5" "S5 Soprano" "Cs05")
add_siren_solo_plugin("S7" "S7 Piccolo" "Cs07")

################################################################################
# Packaging
################################################################################

if(JUCE_COPY_PLUGIN_AFTER_BUILD)
  return()
endif()

if(APPLE)
  include(Packaging/Apple/MakePackage.cmake)
elseif(WIN32)
  include(Packaging/Windows/MakePackage.cmake)
elseif(UNIX AND NOT APPLE)
  include(Packaging/Linux/MakePackage.cmake)
endif()
