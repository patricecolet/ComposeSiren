################################################################################
# Linux Debian Package Configuration
# Based on Packaging/Windows/MakePackage.cmake
################################################################################

################################################################################
# Install targets

# Install Standalone executable
install(
  TARGETS ${BaseTargetName}_Standalone
  RUNTIME DESTINATION bin
  COMPONENT Standalone
)

# Install Resources
install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/Resources/"
  DESTINATION "share/${BaseTargetName}/Resources"
  COMPONENT Standalone
  FILES_MATCHING PATTERN "data*"
)

# Install .desktop file for application menu
install(
  FILES "${CMAKE_CURRENT_LIST_DIR}/composesiren.desktop"
  DESTINATION "share/applications"
  COMPONENT Standalone
)

# Install icon for application menu (using the 40x37 picto as it's smaller)
install(
  FILES "${CMAKE_SOURCE_DIR}/Assets/Picto_Siren_40x37.png"
  DESTINATION "share/pixmaps"
  RENAME "composesiren.png"
  COMPONENT Standalone
)

################################################################################
# Configure CPack for Debian

set(PACKAGING_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/Packaging")

# Define components to package - Only Standalone (exclude JUCE)
set(CPACK_COMPONENTS_ALL Standalone)
set(CPACK_DEB_COMPONENT_INSTALL ON)

# Only install the Standalone component, not JUCE
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};Standalone;/")

# Script to clean JUCE files before packaging
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/CleanJUCEFiles.cmake")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CleanJUCEFiles.cmake" "
# Remove JUCE installation files that shouldn't be in the package
file(REMOVE_RECURSE
    \"\${CPACK_TEMPORARY_INSTALL_DIRECTORY}/usr/include/JUCE-7.0.3\"
    \"\${CPACK_TEMPORARY_INSTALL_DIRECTORY}/usr/lib/cmake/JUCE-7.0.3\"
    \"\${CPACK_TEMPORARY_INSTALL_DIRECTORY}/usr/bin/JUCE-7.0.3\"
)
")

# Basic package info
set(CPACK_PACKAGE_NAME ${BaseTargetName})
set(CPACK_PACKAGE_VENDOR ${VendorName})
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/Packaging/${BaseTargetName}_Installer_artefacts")

# Debian-specific settings
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_NAME "${BaseTargetName}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mecanique Vivante <contact@mecanique-vivante.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "ComposeSiren audio synthesizer
 ComposeSiren is an audio plugin that synthesizes sounds of sirens
 made by Mécanique Vivante. The plugin allows to handle the seven-piece
 Siren Orchestra: two altos (S1 and S2), a bass (S3), a tenor (S4),
 two sopranos (S5 and S6), and a piccolo (S7).")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.mecanique-vivante.com")

# Architecture detection
execute_process(
  COMMAND dpkg --print-architecture
  OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Suggested dependencies (not required, but recommended)
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "jackd2, qjackctl, a2jmidid")

# Package file name format: composesiren_1.5.0_arm64.deb
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# License and documentation
if(EXISTS "${PACKAGING_RESOURCES_DIR}/License.txt")
  set(CPACK_RESOURCE_FILE_LICENSE "${PACKAGING_RESOURCES_DIR}/License.txt")
endif()

if(EXISTS "${PACKAGING_RESOURCES_DIR}/ReadMe.txt")
  set(CPACK_RESOURCE_FILE_README "${PACKAGING_RESOURCES_DIR}/ReadMe.txt")
endif()

################################################################################
# Include CPack and add components

include(CPack)

cpack_add_component(Standalone 
  DISPLAY_NAME "ComposeSiren Standalone Application"
  DESCRIPTION "Standalone audio synthesizer application with siren sample data"
  REQUIRED
)

