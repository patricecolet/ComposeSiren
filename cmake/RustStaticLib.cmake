# ------------------------------------------------------------------------------
# composesiren_add_rust_staticlib: links a Rust staticlib crate into the build.
#
#   composesiren_add_rust_staticlib(
#     TARGET <name>              # the crate's lib name (libname.a), also the
#                                # CMake target: composesiren_mcp, ...
#     CRATE_DIR <dir>            # the directory with Cargo.toml
#     [INCLUDE_DIR <dir>]        # the C headers (default: <CRATE_DIR>/include)
#     [FEATURES <feature>...]    # cargo features
#     [LINK_FRAMEWORKS <name>...] # macOS frameworks the crate needs
#   )
#
# On Apple, cargo is invoked with --target for each CMAKE_OSX_ARCHITECTURES
# entry (one arch for a host-only build, both for MACOS_UNIVERSAL). Several
# architectures are merged with lipo. Corrosion is not used on Apple: it
# builds the host triple unless Rust_CARGO_TARGET is forced, and a leftover
# host .a then fails the linker with "Undefined symbols for architecture …".
# Everywhere else, Corrosion (fetched once, at configure time) imports the crate.
#
# Debug builds use cargo's dev profile, other configurations release.
# MACOSX_DEPLOYMENT_TARGET is passed to cargo when CMAKE_OSX_DEPLOYMENT_TARGET
# is set (rustc rejects an empty one), and RUSTC is the rustc next to that cargo.

include_guard(GLOBAL)

if(APPLE)
  set(COMPOSESIREN_RUST_APPLE_CARGO ON)
  find_program(COMPOSESIREN_CARGO cargo
    HINTS "$ENV{CARGO_HOME}/bin" "$ENV{HOME}/.cargo/bin"
    REQUIRED)
  find_program(COMPOSESIREN_RUSTUP rustup
    HINTS "$ENV{CARGO_HOME}/bin" "$ENV{HOME}/.cargo/bin")
  find_program(COMPOSESIREN_LIPO lipo REQUIRED)

  set(COMPOSESIREN_RUST_INSTALLED_TARGETS "")
  if(COMPOSESIREN_RUSTUP)
    execute_process(
      COMMAND "${COMPOSESIREN_RUSTUP}" target list --installed
      OUTPUT_VARIABLE COMPOSESIREN_RUST_INSTALLED_TARGETS
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
else()
  set(COMPOSESIREN_RUST_APPLE_CARGO OFF)
  # At file scope, not in the function: the variables Corrosion sets stay
  # visible to corrosion_import_crate.
  include(FetchContent)
  FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6.1
  )
  FetchContent_MakeAvailable(Corrosion)
endif()

function(composesiren_add_rust_staticlib)
  cmake_parse_arguments(PARSE_ARGV 0 arg
    ""
    "TARGET;CRATE_DIR;INCLUDE_DIR"
    "FEATURES;LINK_FRAMEWORKS")
  if(arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "composesiren_add_rust_staticlib: unknown arguments ${arg_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT arg_TARGET OR NOT arg_CRATE_DIR)
    message(FATAL_ERROR "composesiren_add_rust_staticlib: TARGET and CRATE_DIR are required")
  endif()
  if(NOT arg_INCLUDE_DIR)
    set(arg_INCLUDE_DIR "${arg_CRATE_DIR}/include")
  endif()

  cmake_path(GET arg_CRATE_DIR FILENAME crate)

  set(cargo_env "")
  if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    list(APPEND cargo_env "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
  endif()
  # The rustc next to the cargo found above (rustup's, whose std is built for an old macOS),
  # not whichever rustc comes first on the PATH: a std built on the build machine (Homebrew's)
  # targets that machine's macOS and the linker warns against CMAKE_OSX_DEPLOYMENT_TARGET.
  if(COMPOSESIREN_RUST_APPLE_CARGO)
    cmake_path(GET COMPOSESIREN_CARGO PARENT_PATH cargo_dir)
    if(EXISTS "${cargo_dir}/rustc")
      list(APPEND cargo_env "RUSTC=${cargo_dir}/rustc")
    endif()
  endif()

  if(COMPOSESIREN_RUST_APPLE_CARGO)
    # -- macOS: cargo --target per architecture, lipo when there are several --
    set(target_dir "${CMAKE_CURRENT_BINARY_DIR}/${crate}-target")
    set(profile "$<IF:$<CONFIG:Debug>,dev,release>")
    set(profile_dir "$<IF:$<CONFIG:Debug>,debug,release>")
    set(lib_name "lib${arg_TARGET}.a")
    set(lib_dir "${CMAKE_CURRENT_BINARY_DIR}/${crate}/$<CONFIG>")
    set(lib "${lib_dir}/${lib_name}")

    set(target_flags "")
    set(arch_libs "")
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
      if(arch STREQUAL "arm64")
        set(triple "aarch64-apple-darwin")
      elseif(arch STREQUAL "x86_64")
        set(triple "x86_64-apple-darwin")
      else()
        message(FATAL_ERROR "${crate}: no Rust target for macOS architecture ${arch}")
      endif()
      if(COMPOSESIREN_RUSTUP AND NOT COMPOSESIREN_RUST_INSTALLED_TARGETS MATCHES "(^|\n)${triple}(\n|$)")
        message(FATAL_ERROR
          "${crate}: needs the Rust target ${triple}. "
          "Install it with: rustup target add ${triple}")
      endif()
      list(APPEND target_flags "--target=${triple}")
      list(APPEND arch_libs "${target_dir}/${triple}/${profile_dir}/${lib_name}")
    endforeach()

    # A list, empty without features: no empty argument reaches cargo.
    set(feature_flags "")
    if(arg_FEATURES)
      list(JOIN arg_FEATURES "," features)
      list(APPEND feature_flags "--features=${features}")
    endif()

    # Always runs: cargo decides what is up to date. lipo -create with one
    # input is a copy, so a host-only build and a universal build share this.
    add_custom_target(${arg_TARGET}_cargo
      COMMAND "${CMAKE_COMMAND}" -E env
        ${cargo_env}
        "CARGO_TARGET_DIR=${target_dir}"
        "${COMPOSESIREN_CARGO}" build
          --manifest-path "${arg_CRATE_DIR}/Cargo.toml"
          --lib
          ${feature_flags}
          "--profile=${profile}"
          ${target_flags}
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${lib_dir}"
      COMMAND "${COMPOSESIREN_LIPO}" -create ${arch_libs} -output "${lib}"
      BYPRODUCTS "${lib}"
      COMMENT "Building ${crate} for ${CMAKE_OSX_ARCHITECTURES}"
      VERBATIM
      USES_TERMINAL
    )

    add_library(${arg_TARGET} INTERFACE)
    target_link_libraries(${arg_TARGET} INTERFACE "${lib}")
    add_dependencies(${arg_TARGET} ${arg_TARGET}_cargo)
  else()
    # -- every other platform: Corrosion ---------------------------------------
    set(corrosion_features "")
    if(arg_FEATURES)
      set(corrosion_features FEATURES ${arg_FEATURES})
    endif()
    corrosion_import_crate(
      MANIFEST_PATH "${arg_CRATE_DIR}/Cargo.toml"
      CRATE_TYPES staticlib
      ${corrosion_features}
    )
    if(NOT TARGET ${arg_TARGET})
      message(FATAL_ERROR "${crate}: Corrosion did not define the target ${arg_TARGET}")
    endif()
    if(cargo_env)
      corrosion_set_env_vars(${arg_TARGET} ${cargo_env})
    endif()
  endif()

  target_include_directories(${arg_TARGET} INTERFACE "${arg_INCLUDE_DIR}")
  if(APPLE)
    foreach(framework IN LISTS arg_LINK_FRAMEWORKS)
      target_link_libraries(${arg_TARGET} INTERFACE "-framework ${framework}")
    endforeach()
  endif()
endfunction()
