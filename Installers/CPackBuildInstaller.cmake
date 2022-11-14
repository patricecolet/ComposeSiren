# utility for finding paths

function(_set_install_folder_from_format FORMAT)
  if(${FORMAT} STREQUAL "Standalone")
    set(INSTALL_FOLDER "/Applications" PARENT_SCOPE)
  elseif(${FORMAT} STREQUAL "AU")
    set(INSTALL_FOLDER "Components" PARENT_SCOPE)
  else()
    set(INSTALL_FOLDER ${FORMAT} PARENT_SCOPE)
  endif()
endfunction()

################################################################################
# sign the targets

if(APPLE AND DEFINED APPLE_DEVELOPER_ID_APPLICATION)
  message("setup signing targets with apple developer id application:")
  message("${APPLE_DEVELOPER_ID_APPLICATION}")

  foreach(FORMAT ${FORMATS})
    get_target_property(ARTEFACTS_DIR ${BaseTargetName}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
    add_custom_command(
      TARGET ${BaseTargetName}_${FORMAT} POST_BUILD
      COMMAND codesign -s ${APPLE_DEVELOPER_ID_APPLICATION} -f ${ARTEFACTS_DIR} --timestamp --deep
      # without VERBATIM option, cmake adds backslashes to escape spaces
      # in strings like APPLE_DEVELOPER_ID_APPLICATION
      VERBATIM)

    # then we store the standalone artefact build path for the cpack post build
    # scripts to find and delete it after packaging :
    # ACTUALLY DOESN'T WORK BECAUSE THE PATH CONTAINS A GENERATOR EXPRESSION
    # TODO : FIND A WORKAROUND (ATM the build dir path is generated below after
    # endforeach() and is used from CpackPostBuildScripts.cmake)

    # if (${FORMAT} STREQUAL "Standalone")
    #   set(STANDALONE_BUILD_DIR ${ARTEFACTS_DIR})
    # endif()
  endforeach()

  set(
    STANDALONE_BUILD_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/${BaseTargetName}_artefacts/Standalone)
endif()

################################################################################
# install the targets

foreach(FORMAT ${FORMATS})
  # will set INSTALL_FOLDER to the right value
  _set_install_folder_from_format(${FORMAT})
  # message(${FORMAT})
  # message(${INSTALL_FOLDER})
  install(
    TARGETS ${BaseTargetName}_${FORMAT}
    # PERMISSIONS
    #   OWNER_EXECUTE OWNER_WRITE OWNER_READ
    #   GROUP_EXECUTE GROUP_WRITE GROUP_READ
    #   WORLD_EXECUTE WORLD_WRITE WORLD_READ  
    DESTINATION ${INSTALL_FOLDER}
    COMPONENT ${FORMAT})
endforeach()

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/Resources/"
  DESTINATION "ComposeSiren/Resources"
  COMPONENT "Resources")

################################################################################
# configure cpack

# this defines the exact list of components we want to package
# (without this some parts of JUCE get included too)
set(COMPONENTS_LIST ${FORMATS})
list(APPEND COMPONENTS_LIST "Resources")
set(CPACK_COMPONENTS_ALL ${COMPONENTS_LIST})

# setup post build script :
# move this into if(APPLE) block ? (only used to remove .app build artefacts)
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/CPackPostBuildScripts.cmake.in
  ${CMAKE_CURRENT_LIST_DIR}/CPackPostBuildScripts.cmake
  @ONLY)

set(CPACK_POST_BUILD_SCRIPTS ${INSTALLERS_PATH}/CPackPostBuildScripts.cmake)

set(CPACK_PACKAGE_NAME ${BaseTargetName})
set(CPACK_PACKAGE_VENDOR "MecaniqueVivante")
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_BINARY_DIR}/Installers)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_LIST_DIR}/License.txt)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_LIST_DIR}/ReadMe.txt)
set(CPACK_RESOURCE_FILE_WELCOME ${CMAKE_CURRENT_LIST_DIR}/Welcome.txt)
set(CPACK_PACKAGE_RELOCATABLE "false")

if(APPLE) ######################################################################
  set(CPACK_GENERATOR "productbuild") ##########################################

  # set(CPACK_PACKAGING_INSTALL_PREFIX "/Library/Audio/Plug-Ins")
  set(CPACK_SET_DESTDIR ON) # because we install Standalone in /Applications
  set(CPACK_INSTALL_PREFIX "/Library/Audio/Plug-ins") # used for relative paths

  # sign the installers :
  if (DEFINED APPLE_DEVELOPER_ID_INSTALLER)
    message("setup signing installer with apple developer id installer:")
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

  # specify icons :
  # set the install/uninstall icon used for the installer itself
  set(CPACK_NSIS_MUI_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\CMakeLogo.ico")
  set( CPACK_NSIS_MUI_UNIICON "${CMake_SOURCE_DIR}/Utilities/Release\\CMakeLogo.ico")
  # set the add/remove programs icon using an installed executable
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\cmake-gui.exe")

endif()

################################################################################
# include cpack and add components

include(CPack)

foreach(FORMAT ${FORMATS})
  cpack_add_component(${FORMAT} DISPLAY_NAME "${FORMAT} Plugin") # DEPENDS "Resources")
endforeach()
cpack_add_component("Resources" DISPLAY_NAME "Shared plugin data" REQUIRED)
