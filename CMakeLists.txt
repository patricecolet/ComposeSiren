cmake_minimum_required(VERSION 3.22.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
  # see : https://forum.juce.com/t/best-way-to-build-with-support-for-older-macos-versions/64879/3
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "minimum supported target macos version")
endif()

if (LINUX)
  # to fix :
  #   relocation R_X86_64_PC32 against symbol
  #   `_ZN10BinaryData18knobbackground_svgE'
  #   can not be used when making a shared object; recompile with -fPIC
  #   https://forum.juce.com/t/linux-cmake-and-fpic/54149/2
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Get the git hash
set(GIT_HASH "Unknown")
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _git_hash_out
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _git_hash_rc
)

if(_git_hash_rc EQUAL 0 AND NOT "${_git_hash_out}" STREQUAL "")
  set(GIT_HASH "${_git_hash_out}")
endif()

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
set(PROJECT_DESCRIPTION "ComposeSiren du ${CURRENT_DATE} sur commit ${GIT_HASH}")

project(ComposeSiren VERSION 2.0.0 DESCRIPTION "${PROJECT_DESCRIPTION}")
set(PROJECT_VERSION_SUFFIX "-beta1") # just for building/packaging purpose
set(BaseTargetName ${PROJECT_NAME})
set(VendorName "Mecanique Vivante")
set(PLUGIN_MANUFACTURER_CODE "McVv")

#set(PLUGIN_TARGETS OneSiren)
set(PLUGIN_TARGETS OneSiren SirenOrchestra)
#set(PLUGIN_TARGETS CustomMix)

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

add_subdirectory(Assets)
add_subdirectory(Source)

################################################################################
# packaging

# if JUCE_COPY_PLUGIN_AFTER_BUILD is set to true, it means we will install non
# signed binaries locally (e.g. for debugging purpose) and we likely don't want
# to build an installer right now, so we stop here :

if(JUCE_COPY_PLUGIN_AFTER_BUILD)
  return()
endif()

if(APPLE) # we generate a cmake subproject to build the installer
  include(Packaging/Apple/MakePackage.cmake)
elseif(WIN32) # we just proceed the regular way
  include(Packaging/Windows/MakePackage.cmake)
  #include(Packaging/Wix/MakePackage.cmake)
endif()
