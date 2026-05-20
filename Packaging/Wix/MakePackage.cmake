################################################################################
# utilities

# Optional multi-target support:
if(NOT DEFINED PLUGIN_TARGETS OR "${PLUGIN_TARGETS}" STREQUAL "")
    set(PLUGIN_TARGETS "${BaseTargetName}")
endif()

list(LENGTH PLUGIN_TARGETS PLUGIN_TARGETS_COUNT)
set(USE_TARGET_FORMAT_COMPONENTS FALSE)
if(PLUGIN_TARGETS_COUNT GREATER 1)
    set(USE_TARGET_FORMAT_COMPONENTS TRUE)
endif()

function(_set_win32_install_folder_from_format FORMAT)
    if(${FORMAT} STREQUAL "Standalone")
        set(INSTALL_FOLDER "${BaseTargetName} ${PROJECT_VERSION}" PARENT_SCOPE)
    elseif(${FORMAT} STREQUAL "VST")
        set(INSTALL_FOLDER "Common Files/VST2" PARENT_SCOPE)
    else()
        set(INSTALL_FOLDER "Common Files/${FORMAT}" PARENT_SCOPE)
    endif()
endfunction()

function(_install_component_from_target_and_format PLUGIN_TARGET_NAME FORMAT)
    _set_win32_install_folder_from_format(${FORMAT})

    if(USE_TARGET_FORMAT_COMPONENTS)
        set(_component_name "${PLUGIN_TARGET_NAME}_${FORMAT}")
    else()
        set(_component_name "${FORMAT}")
    endif()

    install(
            TARGETS ${PLUGIN_TARGET_NAME}_${FORMAT}
            DESTINATION ${INSTALL_FOLDER}
            COMPONENT "${_component_name}"
    )
endfunction()

function(_sign_component_from_format FORMAT)
    # nothing to do here (until we want to sign binaries for windows)
endfunction()

################################################################################
# sign and install the targets

foreach(PLUGIN_TARGET_NAME IN LISTS PLUGIN_TARGETS)
    foreach(FORMAT ${FORMATS})
        get_target_property(ARTEFACTS_DIR ${PLUGIN_TARGET_NAME}_${FORMAT} JUCE_PLUGIN_ARTEFACT_FILE)
        _sign_component_from_format(${FORMAT})
        _install_component_from_target_and_format(${PLUGIN_TARGET_NAME} ${FORMAT})
    endforeach()
endforeach()

if(NOT "${PLUGIN_RESOURCES_DIR}" STREQUAL "")
    set(INSTALL_FOLDER "Common Files/${VendorName}/${BaseTargetName}/Resources")
    install(
            DIRECTORY "${CMAKE_SOURCE_DIR}/Resources/"
            DESTINATION ${INSTALL_FOLDER}
            COMPONENT "Resources"
    )
endif()

################################################################################
# configure cpack

set(PACKAGING_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Packaging")
set(PACKAGING_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/Packaging/Windows")

# this defines the exact list of components we want to package
# (without this some parts of JUCE get included too)
set(COMPONENTS_LIST "")

if(USE_TARGET_FORMAT_COMPONENTS)
    foreach(PLUGIN_TARGET_NAME IN LISTS PLUGIN_TARGETS)
        foreach(FORMAT ${FORMATS})
            list(APPEND COMPONENTS_LIST "${PLUGIN_TARGET_NAME}_${FORMAT}")
        endforeach()
    endforeach()
else()
    set(COMPONENTS_LIST ${FORMATS})
endif()

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
# WIX specific settings

set(CPACK_GENERATOR WIX)

#set(CPACK_WIX_UPGRADE_GUID "F9AAAAE2-D6AF-4EA4-BF46-B3E265400CC7")

include(CPack)

if(USE_TARGET_FORMAT_COMPONENTS)
    foreach(PLUGIN_TARGET_NAME IN LISTS PLUGIN_TARGETS)
        foreach(FORMAT ${FORMATS})
            cpack_add_component("${PLUGIN_TARGET_NAME}_${FORMAT}"
                    DISPLAY_NAME "${PLUGIN_TARGET_NAME} (${FORMAT})"
            )
        endforeach()
    endforeach()
else()
    foreach(FORMAT ${FORMATS})
        cpack_add_component(${FORMAT} DISPLAY_NAME "${FORMAT} Plugin")
    endforeach()
endif()

if(NOT "${PLUGIN_RESOURCES_DIR}" STREQUAL "")
    cpack_add_component("Resources" DISPLAY_NAME "Shared plugin data" REQUIRED)
endif()

message("${COMPONENTS_LIST}")
add_custom_target(dist DEPENDS ${COMPONENTS_LIST})
add_custom_command(TARGET dist POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "== Running CPack (Wix) =="
        COMMAND "${CMAKE_CPACK_COMMAND}" -C "$<IF:$<CONFIG:>,${CMAKE_BUILD_TYPE},$<CONFIG>>"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)