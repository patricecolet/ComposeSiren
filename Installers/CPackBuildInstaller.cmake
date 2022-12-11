################################################################################
# utilities

function(_set_apple_install_folder_from_format FORMAT)
  if(${FORMAT} STREQUAL "Standalone")
    set(INSTALL_FOLDER "/Applications" PARENT_SCOPE)
  elseif(${FORMAT} STREQUAL "AU")
    set(INSTALL_FOLDER "Components" PARENT_SCOPE)
  else()
    set(INSTALL_FOLDER ${FORMAT} PARENT_SCOPE)
  endif()
endfunction()

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
  if(APPLE)
    _set_apple_install_folder_from_format(${FORMAT})
  elseif(WIN32)
    _set_win32_install_folder_from_format(${FORMAT})
  endif()

  install(
    TARGETS ${BaseTargetName}_${FORMAT}
    # PERMISSIONS
    #   OWNER_EXECUTE OWNER_WRITE OWNER_READ
    #   GROUP_EXECUTE GROUP_READ
    #   WORLD_EXECUTE WORLD_READ  
    DESTINATION ${INSTALL_FOLDER}
    # PATTERN "*.ico"
    # PATTERN "*.icns"
    # EXCLUDE
    COMPONENT ${FORMAT}
  )
endfunction()

function(_sign_component_from_format FORMAT)
  if(APPLE AND APPLE_DEVELOPER_ID_APPLICATION)
    get_target_property(ARTEFACTS_DIR ${BaseTargetName}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
    add_custom_command(
      TARGET ${BaseTargetName}_${FORMAT} POST_BUILD
      COMMAND codesign
      -s "${APPLE_DEVELOPER_ID_APPLICATION}"
      -f "${ARTEFACTS_DIR}"
      --options runtime
      --timestamp --deep
      # without VERBATIM option, cmake adds backslashes to escape spaces
      # in strings like APPLE_DEVELOPER_ID_APPLICATION
      VERBATIM
      COMMAND ${CMAKE_COMMAND} -E echo ">>>>> ${BaseTargetName}_${FORMAT} signed in ${ARTEFACTS_DIR}"
    )
  endif()
endfunction()

################################################################################
# sign and install the targets

if(APPLE)
  set(INSTALL_FOLDER "Mecanique Vivante/${BaseTargetName}")
elseif(WIN32)
  set(INSTALL_FOLDER "Common Files/Mecanique Vivante/${BaseTargetName}")
endif()

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/Resources"
  DESTINATION ${INSTALL_FOLDER}
  COMPONENT "Resources"
)

foreach(FORMAT ${FORMATS})
  get_target_property(ARTEFACTS_DIR ${BaseTargetName}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
  # _sign_component_from_format(${FORMAT})
  _install_component_from_format(${FORMAT})
endforeach()


# juce_enable_copy_plugin_step(${BaseTargetName})

################################################################################
# configure cpack

set(INSTALL_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Installers")

# this defines the exact list of components we want to package
# (without this some parts of JUCE get included too)
set(COMPONENTS_LIST ${FORMATS})
list(APPEND COMPONENTS_LIST "Resources")
set(CPACK_COMPONENTS_ALL ${COMPONENTS_LIST})

set(CPACK_PACKAGE_NAME ${BaseTargetName})
set(CPACK_PACKAGE_VENDOR "MecaniqueVivante")
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/Installers/${BaseTargetName}_Installer_artefacts")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/License.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_LIST_DIR}/ReadMe.txt")
set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_LIST_DIR}/Welcome.txt")
set(CPACK_PACKAGE_RELOCATABLE "true")

if(APPLE) ######################################################################
  set(CPACK_GENERATOR "productbuild") ##########################################

  # set(CPACK_PACKAGING_INSTALL_PREFIX "/Library/Audio/Plug-Ins")
  set(CPACK_SET_DESTDIR ON) # because we install Standalone in /Applications
  set(CPACK_INSTALL_PREFIX "/Library/Audio/Plug-ins") # used for relative paths
  set(CPACK_PACKAGE_FILE_NAME "${BaseTargetName}-${PROJECT_VERSION}-Darwin-universal")

  configure_file(
    ${INSTALL_RESOURCES_DIR}/RemoveAppBundles.cmake.in
    ${INSTALL_RESOURCES_DIR}/RemoveAppBundles.cmake
    @ONLY
  )
  configure_file(
    ${INSTALL_RESOURCES_DIR}/Uninstall.applescript.in
    ${INSTALL_RESOURCES_DIR}/Uninstall.applescript
  )
  configure_file(
    ${INSTALL_RESOURCES_DIR}/CreateDmg.sh.in
    ${INSTALL_RESOURCES_DIR}/CreateDmg.sh
    FILE_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
  )
  configure_file(
    ${INSTALL_RESOURCES_DIR}/NotarizeDmg.sh.in
    ${INSTALL_RESOURCES_DIR}/NotarizeDmg.sh
    FILE_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
  )

  # sign the installers :
  if (APPLE_DEVELOPER_ID_INSTALLER)
    message("setup signing installers with apple developer id installer:")
    message("${APPLE_DEVELOPER_ID_INSTALLER}")

    set(CPACK_PRODUCTBUILD_IDENTITY_NAME ${APPLE_DEVELOPER_ID_INSTALLER})
    set(CPACK_PKGBUILD_IDENTITY_NAME ${APPLE_DEVELOPER_ID_INSTALLER})
  endif()

  # potentially useful settings (but makes resources path harder to guess) :
  # set(CPACK_PRODUCTBUILD_DOMAINS TRUE)
  # set(CPACK_PRODUCTBUILD_DOMAINS_USER TRUE)
  # set(CPACK_PRODUCTBUILD_DOMAINS_ANYWHERE TRUE)
  # set(CPACK_PRODUCTBUILD_DOMAINS_ROOT TRUE)

elseif(WIN32) ##################################################################
  set(CPACK_GENERATOR "NSIS") ##################################################

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

  # TODO - specify icons :
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

endif()

# setup pre build script (used to remove undesired icon files from tmp install)
# if(WIN32)
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/CPackPreBuildScripts.cmake.in
  ${CMAKE_CURRENT_LIST_DIR}/CPackPreBuildScripts.cmake
  @ONLY # without @ONLY, regular variables in the .in file get discarded
)
set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_CURRENT_LIST_DIR}/CPackPreBuildScripts.cmake)
# endif(WIN32)

# setup post build script (only used to remove .app build artefacts)
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/CPackPostBuildScripts.cmake.in
  ${CMAKE_CURRENT_LIST_DIR}/CPackPostBuildScripts.cmake
  @ONLY # without @ONLY, regular variables in the .in file get discarded
)
set(CPACK_POST_BUILD_SCRIPTS ${CMAKE_CURRENT_LIST_DIR}/CPackPostBuildScripts.cmake)

# if(APPLE)
# set(CPACK_POST_BUILD_SCRIPTS ${INSTALL_RESOURCES_DIR}/RemoveAppBundles.cmake)
# endif(APPLE)

################################################################################
# include cpack and add components

include(CPack)

foreach(FORMAT ${FORMATS})
  cpack_add_component(${FORMAT} DISPLAY_NAME "${FORMAT} Plugin") # DEPENDS "Resources")
endforeach()
cpack_add_component("Resources" DISPLAY_NAME "Shared plugin data" REQUIRED)
