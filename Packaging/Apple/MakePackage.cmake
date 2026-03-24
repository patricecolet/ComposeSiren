# keep track of the paths to all generated binaries for later packaging
# TODO : find a cleaner way to work with targets (e.g. by exporting them rather
# than retrieving their paths in a clumsy way, or even better do this without
# generating an extra cmake project)
# see : https://stackoverflow.com/a/64882742/3810717 and dig doc

# Optional multi-target support:
# - If PLUGIN_TARGETS is not provided, we keep backward compatibility and
#   package a single plugin target named BaseTargetName.
if(NOT DEFINED PLUGIN_TARGETS OR "${PLUGIN_TARGETS}" STREQUAL "")
  set(PLUGIN_TARGETS "${BaseTargetName}")
endif()

# Generate artefact path files for each target+format
foreach(PLUGIN_TARGET_NAME IN LISTS PLUGIN_TARGETS)
  foreach(FORMAT ${FORMATS})
    get_target_property(ARTEFACTS_DIR ${PLUGIN_TARGET_NAME}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
    if(CMAKE_CONFIGURATION_TYPES)
      set(MULTI_CONFIG TRUE)
      file(
        GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TARGET_NAME}_${FORMAT}_$<CONFIG>_path"
        CONTENT "${ARTEFACTS_DIR}"
      )
    else()
      set(MULTI_CONFIG FALSE)
      file(
        GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TARGET_NAME}_${FORMAT}_path"
        CONTENT "${ARTEFACTS_DIR}"
      )
    endif()
  endforeach()
endforeach()

# configure the cmake package subproject
set(PACKAGING_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Packaging")
set(PACKAGING_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/Packaging/Apple")
set(PACKAGING_PROJECT_SOURCE_DIR "${CMAKE_BINARY_DIR}/Packaging")

configure_file(
  ${PACKAGING_SCRIPTS_DIR}/Packaging.CMakeLists.cmake.in
  ${PACKAGING_PROJECT_SOURCE_DIR}/CMakeLists.txt
  @ONLY
)

# wait until all targets are built, then build the package subproject
set(ALL_TARGETS "")
foreach(PLUGIN_TARGET_NAME IN LISTS PLUGIN_TARGETS)
  foreach(FORMAT ${FORMATS})
    list(APPEND ALL_TARGETS "${PLUGIN_TARGET_NAME}_${FORMAT}")
  endforeach()
endforeach()

add_custom_target(package DEPENDS ${ALL_TARGETS})
add_custom_command(
  TARGET package POST_BUILD
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

  COMMAND ${CMAKE_COMMAND} -E echo "========== FIXING STANDALONE RELOCATABLE"
  COMMAND ${PACKAGING_PROJECT_SOURCE_DIR}/FixAllStandalonesRelocatable.sh
  COMMAND ${CMAKE_COMMAND} -E echo "========== STANDALONE RELOCATABLE FIXED"

  # clean build artefacts (this was before we managed to disable app relocatability)
  # this should be removed once we're sure it's not useful at all anymore,
  # together with RemoveAppBundles.cmake.in
  # COMMAND ${CMAKE_COMMAND} -E echo "========== REMOVING .app ARTEFACTS"
  # COMMAND ${CMAKE_COMMAND} -P ${PACKAGING_PROJECT_SOURCE_DIR}/RemoveAppBundles.cmake
  # COMMAND ${CMAKE_COMMAND} -E echo "========== .app ARTEFACTS REMOVED"

  # create dmg containing installer and uninstaller
  COMMAND ${CMAKE_COMMAND} -E echo "========== CREATING DMG"
  COMMAND ${PACKAGING_PROJECT_SOURCE_DIR}/CreateDmg.sh
  COMMAND ${CMAKE_COMMAND} -E echo "========== DMG CREATED"
)

# Meta-Target that acts as an "all/ALL_BUILD" target
# as not all environments provide it by default like Ninja
# (e.g. CLion / CI)
add_custom_target(dist DEPENDS package)

if(ENABLE_NOTARIZATION)
  # fail fast if notarization profile has not been provided
  if (NOT DEFINED APPLE_NOTARIZATION_KEYCHAIN_PROFILE OR
      APPLE_NOTARIZATION_KEYCHAIN_PROFILE STREQUAL "")
    message(FATAL_ERROR
      "ENABLE_NOTARIZATION is true but APPLE_NOTARIZATION_KEYCHAIN_PROFILE
      is not set. "
      "Set -DAPPLE_NOTARIZATION_KEYCHAIN_PROFILE=<profile> in the command
      or in the cmake config file"
    )
  endif()
  # notarize the generated dmg for painless distribution
  add_custom_target(notarize DEPENDS package)
  add_custom_command(
    TARGET notarize POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "========== NOTARIZING DMG"
    COMMAND ${PACKAGING_PROJECT_SOURCE_DIR}/NotarizeDmg.sh
    COMMAND ${CMAKE_COMMAND} -E echo "========== DMG NOTARIZED"
  )
  add_dependencies(dist notarize)
endif()
