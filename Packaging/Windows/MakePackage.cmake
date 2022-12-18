################################################################################
# utilities

function(_set_win32_install_folder_from_format FORMAT)
  if(${FORMAT} STREQUAL "Standalone")
	  set(INSTALL_FOLDER "${BaseTargetName} ${PROJECT_VERSION}" PARENT_SCOPE)
  elseif(${FORMAT} STREQUAL "VST")
	  set(INSTALL_FOLDER "Common Files/VST2" PARENT_SCOPE)
  else()
	  set(INSTALL_FOLDER "Common Files/${FORMAT}" PARENT_SCOPE)
  endif()
endfunction()

function(_install_component_from_format FORMAT)
  _set_win32_install_folder_from_format(${FORMAT})

  install(
    TARGETS ${BaseTargetName}_${FORMAT}
    DESTINATION ${INSTALL_FOLDER}
    COMPONENT ${FORMAT}
  )
endfunction()

function(_sign_component_from_format FORMAT)
  # nothing to do here (until we want to sign binaries for windows)
endfunction()

################################################################################
# sign and install the targets

foreach(FORMAT ${FORMATS})
  get_target_property(ARTEFACTS_DIR ${BaseTargetName}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
  _sign_component_from_format(${FORMAT})
  _install_component_from_format(${FORMAT})
endforeach()

if(NOT "${PLUGIN_RESOURCES_DIR}" STREQUAL "")
  set(INSTALL_FOLDER "Common Files/${VendorName}/${BaseTargetName}/Resources")
endif()

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/Resources/"
  DESTINATION ${INSTALL_FOLDER}
  COMPONENT "Resources"
)

################################################################################
# configure cpack

set(PACKAGING_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Packaging")
set(PACKAGING_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/Packaging/Windows")

# this defines the exact list of components we want to package
# (without this some parts of JUCE get included too)
set(COMPONENTS_LIST ${FORMATS})

if(NOT "${PLUGIN_RESOURCES_DIR}" STREQUAL "")
  list(APPEND COMPONENTS_LIST "Resources")
endif()

set(CPACK_COMPONENTS_ALL ${COMPONENTS_LIST})

set(CPACK_PACKAGE_NAME ${BaseTargetName})
set(CPACK_PACKAGE_VENDOR ${VendorName})
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/Packaging/${BaseTargetName}_Installer_artefacts")
set(CPACK_RESOURCE_FILE_LICENSE "${PACKAGING_RESOURCES_DIR}/License.txt")
set(CPACK_RESOURCE_FILE_README "${PACKAGING_RESOURCES_DIR}/ReadMe.txt")
set(CPACK_RESOURCE_FILE_WELCOME "${PACKAGING_RESOURCES_DIR}/Welcome.txt")
set(CPACK_PACKAGE_RELOCATABLE "true")

################################################################################
# NSIS specific settings

set(CPACK_GENERATOR "NSIS")

# this allows us to use our custom NSIS.template.in, changing the
# uninstaller's $INSTDIR value to "$INSTDIR\.." so that we can install it in
# "$INSTDIR\${CPACK_NSIS_PACKAGE_INSTALL_DIRECTORY}"
# we do this because we want $INSTDIR to be "C:\Program Files" so that we can
# install plugins in "Common Files" and Standalone + uninstaller in
# ${CPACK_NSIS_PACKAGE_INSTALL_DIRECTORY}
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# if we want to add something to the path (might be useful for resources)
# set(CPACK_NSIS_MODIFY_PATH ON)

# we set $INSTDIR to "C:\Program Files"
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
# if not specified, CPACK_PACKAGE_INSTALL_DIRECTORY defaults to
# ${PROJECT_NAME}-${PROJECT_VERSION}, we want to avoid that
set(CPACK_PACKAGE_INSTALL_DIRECTORY "")

# we create our own install directory name
set(CPACK_NSIS_PACKAGE_INSTALL_DIRECTORY "${BaseTargetName} ${PROJECT_VERSION}")
set(CPACK_NSIS_PACKAGE_NAME "${BaseTargetName} ${PROJECT_VERSION}")

# CPACK_NSIS_DISPLAY_NAME and CPACK_PACKAGE_INSTALL_REGISTRY_KEY are required
# to create an entry in windows' add/remove programs list :
set(CPACK_NSIS_DISPLAY_NAME ${CPACK_NSIS_PACKAGE_NAME})
# default registry key is based on the install directory so we need to provide
# it explicitly :
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${BaseTargetName} ${PROJECT_VERSION}")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall ${BaseTargetName} ${PROJECT_VERSION}")

# TODO - use CPACK to add NSIS commands to create and remove icons in startmenu
# For now this is achieved directly into the NSIS.template.in file (both approaches look as convoluted)
# set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${CPACK_NSIS_PACKAGE_NAME}.lnk' '$INSTDIR\\\\${CPACK_NSIS_PACKAGE_INSTALL_DIRECTORY}\\\${CPACK_PACKAGE_NAME}.exe'")
# set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${CPACK_NSIS_PACKAGE_NAME}.lnk'")

# specify icons :
# set the install/uninstall icon used for the installer itself
# set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/Resources/Picto_Siren_40x37.ico")
# leave the default nsis uninstaller icon until we have a nice one
# set(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/Resources/Picto_Siren_40x37.ico")
# set the add/remove programs icon using an installed executable
set(CPACK_NSIS_INSTALLED_ICON_NAME "${BaseTargetName}.exe")

# are these not helpful ? (shouldn't they be CPACK_NSIS_... vars instead ?)
# set(CPACK_Standalone_INSTALL_DIRECTORY "${BaseTargetName} ${PROJECT_VERSION}")
# set(CPACK_VST_INSTALL_DIRECTORY "Common Files/VST2")
# set(CPACK_VST3_INSTALL_DIRECTORY "Common Files/VST3")

################################################################################
# set up cpack pre / post build scripts

configure_file(
  ${PACKAGING_SCRIPTS_DIR}/CPackPreBuildScripts.cmake.in
  ${CMAKE_BINARY_DIR}/CPackPreBuildScripts.cmake
  @ONLY
)
set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/CPackPreBuildScripts.cmake)

configure_file(
  ${PACKAGING_SCRIPTS_DIR}/CPackPostBuildScripts.cmake.in
  ${CMAKE_BINARY_DIR}/CPackPostBuildScripts.cmake
  @ONLY
)
set(CPACK_POST_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/CPackPostBuildScripts.cmake)

################################################################################
# include cpack and add components

include(CPack)

foreach(FORMAT ${FORMATS})
  cpack_add_component(${FORMAT} DISPLAY_NAME "${FORMAT} Plugin")
endforeach()

if(NOT "${PLUGIN_RESOURCES_DIR}" STREQUAL "")
  cpack_add_component("Resources" DISPLAY_NAME "Shared plugin data" REQUIRED)
endif()
