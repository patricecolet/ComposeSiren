# keep track of the paths to all generated binaries for later packaging
# TODO : find a cleaner way to work with targets (e.g. by exporting them rather
# than retrieving their paths in a clumsy way, or even better do this without
# generating an extra cmake project)
# see : https://stackoverflow.com/a/64882742/3810717 and dig doc

set(ALL_TARGETS "")

foreach(FORMAT ${FORMATS})
  get_target_property(ARTEFACTS_DIR ${BaseTargetName}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
  list(APPEND ALL_TARGETS "${BaseTargetName}_${FORMAT}")
  if(CMAKE_CONFIGURATION_TYPES)
    set(MULTI_CONFIG TRUE)
    message(STATUS "Multi-config types : ${CMAKE_CONFIGURATION_TYPES}")
    file(
      GENERATE
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BaseTargetName}_${FORMAT}_$<CONFIG>_path"
      CONTENT "${ARTEFACTS_DIR}"
    )
  else()
    set(MULTI_CONFIG FALSE)
    message(STATUS "Single-config type: ${CMAKE_BUILD_TYPE}")
    file(
      GENERATE
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BaseTargetName}_${FORMAT}_path"
      CONTENT "${ARTEFACTS_DIR}"
    )
  endif()
endforeach()

# configure the cmake package installer subproject

set(PLUGIN_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Resources")
set(INSTALL_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Installers")
set(SubProjectSourceDir "${CMAKE_BINARY_DIR}/Installers")

configure_file(
  ${INSTALL_RESOURCES_DIR}/Installer.CMakeLists.cmake.in
  ${SubProjectSourceDir}/CMakeLists.txt
  @ONLY
)

# wait until all targets are built, then build the cmake subproject

# add_custom_target(CopyResources ALL DEPENDS ${ALL_TARGETS})
# file(GLOB rsrcFiles "${CMAKE_SOURCE_DIR}/Resources/data*")
# foreach(rsrcFile ${rsrcFiles})
#   add_custom_command(TARGET CopyResources PRE_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy ${rsrcFile} "${CMAKE_BINARY_DIR}/Installers"
#   )
# endforeach()

# add_custom_command(
#   TARGET CopyResources POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_directory
#   "${CMAKE_SOURCE_DIR}/Resources"
#   "${CMAKE_BINARY_DIR}/Installers"
# )
# list(APPEND ALL_TARGETS "CopyResources")

message(STATUS "${ALL_TARGETS}")
add_custom_target(Packaging ALL DEPENDS ${ALL_TARGETS})
# add_custom_target(Packaging ALL DEPENDS CopyResources)
add_custom_command(
  TARGET Packaging POST_BUILD

  COMMAND ${CMAKE_COMMAND} -E echo ""
  COMMAND ${CMAKE_COMMAND} -E echo "========== START PACKAGING ..."
  COMMAND ${CMAKE_COMMAND} -E echo ""

  COMMAND ${CMAKE_COMMAND}
  -B .
  -G ${CMAKE_GENERATOR}
  -DCMAKE_BUILD_TYPE=Release
  WORKING_DIRECTORY ${SubProjectSourceDir}

  COMMAND ${CMAKE_COMMAND}
  --build . --config Release
  WORKING_DIRECTORY ${SubProjectSourceDir}

  COMMAND ${CMAKE_CPACK_COMMAND}
  --config CPackConfig.cmake
  WORKING_DIRECTORY ${SubProjectSourceDir}

  COMMAND ${CMAKE_COMMAND} -E echo ""
  COMMAND ${CMAKE_COMMAND} -E echo "========== PACKAGING DONE !"
  COMMAND ${CMAKE_COMMAND} -E echo ""
)

if(APPLE)
  add_custom_target(PostPackaging ALL DEPENDS Packaging)
  add_custom_command(
    TARGET PostPackaging POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "========== REMOVING .app ARTEFACTS ..."
    COMMAND ${CMAKE_COMMAND} -E echo ""

    COMMAND ${CMAKE_COMMAND} -P ${INSTALL_RESOURCES_DIR}/RemoveAppBundles.cmake

    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "========== CREATING DMG ..."
    COMMAND ${CMAKE_COMMAND} -E echo ""

    COMMAND ${INSTALL_RESOURCES_DIR}/CreateDmg.sh

    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "========== DMG CREATED !"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "========== NOTARIZING DMG ..."
    COMMAND ${CMAKE_COMMAND} -E echo ""

    COMMAND ${INSTALL_RESOURCES_DIR}/NotarizeDmg.sh

    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "========== DMG NOTARIZED !"
    COMMAND ${CMAKE_COMMAND} -E echo ""
  )
endif()
