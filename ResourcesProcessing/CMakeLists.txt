# Resources preprocessing (F#, .NET SDK)
#
# fix_data.fsx writes the crossfade-fixed data files (FixData.fs) from ORIGINAL_RESOURCES_DIR into
# PROCESSED_RESOURCES_DIR, a complete drop-in for Resources/ (Config.cmake / CMakeLists.txt define
# both). Needs `dotnet` (the .NET SDK, for `dotnet fsi`) on the PATH or in DOTNET_EXECUTABLE.

set(RES_INPUT_DIR "${ORIGINAL_RESOURCES_DIR}")
set(RES_OUTPUT_DIR "${PROCESSED_RESOURCES_DIR}")
set(RES_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/fix_data.fsx")
set(RES_SCRIPT_DEPS "${CMAKE_CURRENT_LIST_DIR}/FixData.fs")

find_program(DOTNET_EXECUTABLE dotnet)
if(NOT DOTNET_EXECUTABLE)
  message(FATAL_ERROR
    "PROCESS_RESOURCES needs the .NET SDK (dotnet fsi runs ResourcesProcessing/fix_data.fsx): "
    "install it from https://dot.net, or set DOTNET_EXECUTABLE, or configure with -DPROCESS_RESOURCES=OFF "
    "to use the original Resources/.")
endif()

# Discover inputs (configure-time listing; rerun cmake if files are added/removed)
file(GLOB RES_INPUT_FILES
        "${RES_INPUT_DIR}/dataAmpS*"
        "${RES_INPUT_DIR}/dataFreqS*"
        "${RES_INPUT_DIR}/datadureTabsS*"
        "${RES_INPUT_DIR}/dataVectorInterval*")

# A stamp file stands for all generated outputs
set(RES_STAMP "${RES_OUTPUT_DIR}/.generated.stamp")

add_custom_command(
        OUTPUT "${RES_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RES_OUTPUT_DIR}"
        COMMAND "${DOTNET_EXECUTABLE}" fsi "${RES_SCRIPT}"
        --resources "${RES_INPUT_DIR}"
        --output "${RES_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${RES_STAMP}"
        DEPENDS "${RES_SCRIPT}" ${RES_SCRIPT_DEPS} ${RES_INPUT_FILES}
        COMMENT "Preprocessing Resources (non-destructive) -> ${RES_OUTPUT_DIR}"
        VERBATIM
)

# Public target to (re)generate processed resources
add_custom_target(process_resources
        DEPENDS "${RES_STAMP}"
)

foreach(PLUGIN_TARGET IN LISTS PLUGIN_TARGETS)
    foreach(FORMAT IN LISTS FORMATS)
        set(target "${PLUGIN_TARGET}_${FORMAT}")
        if(TARGET "${target}")
            add_dependencies("${target}" process_resources)
        endif()
    endforeach()
endforeach()
