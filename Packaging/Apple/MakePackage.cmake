# keep track of the paths to all generated binaries for later packaging
# TODO : find a cleaner way to work with targets (e.g. by exporting them rather
# than retrieving their paths in a clumsy way, or even better do this without
# generating an extra cmake project)
# see : https://stackoverflow.com/a/64882742/3810717 and dig doc

# List of all plugin targets (Orchestra + Solo)
# Option pour construire seulement Orchestra (sans Solo)
option(BUILD_ONLY_ORCHESTRA "Build only Orchestra package without Solo models" OFF)
if(BUILD_ONLY_ORCHESTRA)
  set(ALL_PLUGIN_TARGETS ${BaseTargetName})
else()
  set(ALL_PLUGIN_TARGETS ${BaseTargetName})
  set(SOLO_MODELS "S1" "S3" "S4" "S5" "S7")
  foreach(MODEL ${SOLO_MODELS})
    list(APPEND ALL_PLUGIN_TARGETS "ComposeSiren_${MODEL}")
  endforeach()
endif()

foreach(PLUGIN_TARGET ${ALL_PLUGIN_TARGETS})
  foreach(FORMAT ${FORMATS})
    get_target_property(ARTEFACTS_DIR ${PLUGIN_TARGET}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
    if(ARTEFACTS_DIR)
      if(CMAKE_CONFIGURATION_TYPES)
        set(MULTI_CONFIG TRUE)
        file(
          GENERATE
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TARGET}_${FORMAT}_$<CONFIG>_path"
          CONTENT "${ARTEFACTS_DIR}"
        )
      else()
        set(MULTI_CONFIG FALSE)
        file(
          GENERATE
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TARGET}_${FORMAT}_path"
          CONTENT "${ARTEFACTS_DIR}"
        )
      endif()
    endif()
  endforeach()
endforeach()

# configure the cmake package subproject
set(PACKAGING_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Packaging")
set(PACKAGING_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/Packaging/Apple")
set(PACKAGING_PROJECT_SOURCE_DIR "${CMAKE_BINARY_DIR}/Packaging")

# Copy path files to Packaging directory so the packaging subproject can find them
file(MAKE_DIRECTORY ${PACKAGING_PROJECT_SOURCE_DIR})
foreach(PLUGIN_TARGET ${ALL_PLUGIN_TARGETS})
  foreach(FORMAT ${FORMATS})
    if(CMAKE_CONFIGURATION_TYPES)
      foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
        set(SOURCE_PATH_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TARGET}_${FORMAT}_${CONFIG}_path")
        if(EXISTS "${SOURCE_PATH_FILE}")
          file(COPY "${SOURCE_PATH_FILE}" DESTINATION ${PACKAGING_PROJECT_SOURCE_DIR})
        endif()
      endforeach()
    else()
      set(SOURCE_PATH_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TARGET}_${FORMAT}_path")
      if(EXISTS "${SOURCE_PATH_FILE}")
        file(COPY "${SOURCE_PATH_FILE}" DESTINATION ${PACKAGING_PROJECT_SOURCE_DIR})
      endif()
    endif()
  endforeach()
endforeach()

# Pass variables to the packaging template
# PLUGIN_RESOURCES_DIR: directory containing plugin resources (from Config.cmake)
# CMAKE_SOURCE_DIR: source directory of the main project (for Resources path)
if(NOT DEFINED PLUGIN_RESOURCES_DIR)
  set(PLUGIN_RESOURCES_DIR "")
endif()

# CMAKE_SOURCE_DIR will be the source dir of the main project in the template
# We need to pass it explicitly since the packaging subproject has its own CMAKE_SOURCE_DIR
set(MAIN_CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
# Pass PACKAGING_SCRIPTS_DIR so the template can find Distribution.xml.in
set(PACKAGING_SCRIPTS_DIR_FOR_TEMPLATE "${PACKAGING_SCRIPTS_DIR}")
# Pass version variables explicitly to ensure they're available in the template
set(PROJECT_VERSION_MAJOR_FOR_TEMPLATE "${PROJECT_VERSION_MAJOR}")
set(PROJECT_VERSION_MINOR_FOR_TEMPLATE "${PROJECT_VERSION_MINOR}")
set(PROJECT_VERSION_PATCH_FOR_TEMPLATE "${PROJECT_VERSION_PATCH}")
set(CMAKE_PROJECT_VERSION_FOR_TEMPLATE "${CMAKE_PROJECT_VERSION}")
set(BUILD_ONLY_ORCHESTRA_FOR_TEMPLATE "${BUILD_ONLY_ORCHESTRA}")

configure_file(
  ${PACKAGING_SCRIPTS_DIR}/Packaging.CMakeLists.cmake.in
  ${PACKAGING_PROJECT_SOURCE_DIR}/CMakeLists.txt
  @ONLY
)

# wait until all targets are built, then build the package subproject
set(ALL_TARGETS "")
foreach(PLUGIN_TARGET ${ALL_PLUGIN_TARGETS})
  foreach(FORMAT ${FORMATS})
    if(TARGET "${PLUGIN_TARGET}_${FORMAT}")
      list(APPEND ALL_TARGETS "${PLUGIN_TARGET}_${FORMAT}")
    endif()
  endforeach()
endforeach()

add_custom_target(Packaging ALL DEPENDS ${ALL_TARGETS})
add_custom_command(
  TARGET Packaging POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "========== CREATING INSTALLER"
  COMMAND ${CMAKE_COMMAND}
    -B .
    -G ${CMAKE_GENERATOR}
    -DCMAKE_BUILD_TYPE=Release
    WORKING_DIRECTORY ${PACKAGING_PROJECT_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND}
    --build . --config Release
    WORKING_DIRECTORY ${PACKAGING_PROJECT_SOURCE_DIR}
  COMMAND ${CMAKE_CPACK_COMMAND}
    --config CPackConfig.cmake
    WORKING_DIRECTORY ${PACKAGING_PROJECT_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} -E echo "========== INSTALLER CREATED"

  # Corriger les PackageInfo des sous-packages AVANT de reconstruire le PKG final
  # C'est crucial : les PackageInfo doivent avoir hostArchitectures="x86_64 arm64"
  # Le PROJECT_NAME dans le packaging subproject est ${BaseTargetName}_Installer
  COMMAND ${CMAKE_COMMAND} -E echo "========== FIXING PACKAGEINFO FORMAT IN SUB-PACKAGES"
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/fix_pkginfo_before_productbuild.sh ${PACKAGING_PROJECT_SOURCE_DIR}/${BaseTargetName}_Installer_artefacts ${BaseTargetName}_Installer
  COMMAND ${CMAKE_COMMAND} -E echo "========== PACKAGEINFO FORMAT FIXED IN SUB-PACKAGES"
  
  # Reconstruire le PKG principal avec les PackageInfo corrigés
  COMMAND ${CMAKE_COMMAND} -E echo "========== REBUILDING PKG WITH FIXED PACKAGEINFO"
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/rebuild_pkg_with_fixed_packageinfo.sh ${PACKAGING_PROJECT_SOURCE_DIR}/${BaseTargetName}_Installer_artefacts ${BaseTargetName}_Installer
  COMMAND ${CMAKE_COMMAND} -E echo "========== PKG REBUILT WITH FIXED PACKAGEINFO"

  # clean build artefacts
  COMMAND ${CMAKE_COMMAND} -E echo "========== REMOVING .app ARTEFACTS"
  COMMAND ${CMAKE_COMMAND} -P ${PACKAGING_PROJECT_SOURCE_DIR}/RemoveAppBundles.cmake
  COMMAND ${CMAKE_COMMAND} -E echo "========== .app ARTEFACTS REMOVED"

  # create dmg containing installer and uninstaller
  COMMAND ${CMAKE_COMMAND} -E echo "========== CREATING DMG"
  COMMAND ${PACKAGING_PROJECT_SOURCE_DIR}/CreateDmg.sh
  COMMAND ${CMAKE_COMMAND} -E echo "========== DMG CREATED"
)

if(${ENABLE_NOTARIZATION})
  # notarize the generated dmg for painless distribution
  add_custom_target(Notarization ALL DEPENDS Packaging)
  add_custom_command(
    TARGET Notarization POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "========== NOTARIZING DMG"
    COMMAND ${PACKAGING_PROJECT_SOURCE_DIR}/NotarizeDmg.sh
    COMMAND ${CMAKE_COMMAND} -E echo "========== DMG NOTARIZED"
  )
endif()
