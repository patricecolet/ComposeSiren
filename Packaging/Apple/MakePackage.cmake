# keep track of the paths to all generated binaries for later packaging
# TODO : find a cleaner way to work with targets (e.g. by exporting them rather
# than retrieving their paths in a clumsy way, or even better do this without
# generating an extra cmake project)
# see : https://stackoverflow.com/a/64882742/3810717 and dig doc

foreach(FORMAT ${FORMATS})
  get_target_property(ARTEFACTS_DIR ${BaseTargetName}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
  if(CMAKE_CONFIGURATION_TYPES)
    set(MULTI_CONFIG TRUE)
    file(
      GENERATE
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BaseTargetName}_${FORMAT}_$<CONFIG>_path"
      CONTENT "${ARTEFACTS_DIR}"
    )
  else()
    set(MULTI_CONFIG FALSE)
    file(
      GENERATE
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BaseTargetName}_${FORMAT}_path"
      CONTENT "${ARTEFACTS_DIR}"
    )
  endif()
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
foreach(FORMAT ${FORMATS})
  list(APPEND ALL_TARGETS "${BaseTargetName}_${FORMAT}")
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
