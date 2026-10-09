# MetaHookSv aggregator
#
# This repository aggregates the MetaHookSv components, each of which lives in
# its own git submodule and is a self-contained CMake project:
#
#   MetaHook/          core launcher / SDK host
#   Plugins/*          game plugins
#   PluginLibs/*       shared plugin libraries
#   toolsrc/*          standalone tools (CMake-based ones only)
#
# Every enabled component is added to this single CMake tree with
# add_subdirectory(), so one configure produces one solution holding the real
# component targets. Components therefore stay includable as subprojects:
#   - shared vendor targets (libglew_static, FreeImage) are added only when no
#     sibling component has added them yet (`if(NOT TARGET ...)`);
#   - imported targets stay directory-scoped (no GLOBAL);
#   - per-component trees such as the synchronized gamedata are anchored to
#     PROJECT_BINARY_DIR, not CMAKE_BINARY_DIR;
#   - generic cache variables (for example BUILD_SHARED_LIBS) are not forced.
#
# Usage (MSVC x86 / Win32 is required by every component). Configure one build
# tree per configuration under build/<arch>/<config> so the default install
# prefix can carry the same arch/config (see the layout block below):
#   cmake -S . -B build/x86/Release -A Win32
#   cmake --build build/x86/Release --config Release
#   cmake --install build/x86/Release --config Release   # stages into install/x86/Release
#
# The aggregated sibling sources are injected per component (see the registry
# below) so components consume the local submodules instead of downloading
# their own copies.

cmake_minimum_required(VERSION 3.21)

include(cmake/Components.cmake)
option(FORMAT_VALIDATION_ONLY "Configure only clang-format targets without native dependencies" OFF)
if(FORMAT_VALIDATION_ONLY)
    project(MetaHookSvFormatValidation LANGUAGES NONE)
    include(cmake/FormatValidation.cmake)
    return()
endif()

project(MetaHookSv LANGUAGES C CXX RC)

# Allow components to group their targets into IDE solution folders via the
# target FOLDER property (a no-op unless the USE_FOLDERS global property is on).
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "MetaHookSv components require Windows MSVC x86. Configure with -A Win32.")
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(METAHOOKSV_INIT_SUBMODULES "Run 'git submodule update --init --recursive' at configure time" ON)

# ---------------------------------------------------------------------------
# Shared build/install layout: build/<arch>/<config> and install/<arch>/<config>.
#
# Configure one CMake tree per configuration (for example -B build/x86/Release),
# so the configuration is already known at configure time. That is required:
# a multi-config generator cannot expand $<CONFIG> inside CMAKE_INSTALL_PREFIX,
# because the generated cmake_install.cmake runs with CMAKE_INSTALL_PREFIX
# already defined and would install into a literal "$<CONFIG>" directory.
# ---------------------------------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(METAHOOKSV_BUILD_ARCH "x86")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(METAHOOKSV_BUILD_ARCH "x64")
else()
    message(FATAL_ERROR "Unsupported pointer size ${CMAKE_SIZEOF_VOID_P}; expected 4 (x86) or 8 (x64).")
endif()

# Derive the configuration from the build tree leaf (build/x86/<config>); fall
# back to CMAKE_BUILD_TYPE for single-config generators, then to Release.
get_filename_component(_metahooksv_build_leaf "${CMAKE_BINARY_DIR}" NAME)
if(_metahooksv_build_leaf MATCHES "^(Debug|Release)$")
    set(METAHOOKSV_BUILD_CONFIG "${_metahooksv_build_leaf}")
elseif(CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    set(METAHOOKSV_BUILD_CONFIG "${CMAKE_BUILD_TYPE}")
else()
    set(METAHOOKSV_BUILD_CONFIG "Release")
endif()
unset(_metahooksv_build_leaf)

# Stage every component into one shared prefix unless one was chosen explicitly.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install/${METAHOOKSV_BUILD_ARCH}/${METAHOOKSV_BUILD_CONFIG}"
        CACHE PATH "Shared install prefix for all components" FORCE)
elseif(DEFINED CACHE{METAHOOKSV_INSTALL_PREFIX})
    # Build trees from the former ExternalProject superbuild kept the shared
    # prefix in METAHOOKSV_INSTALL_PREFIX; carry it over once.
    set(CMAKE_INSTALL_PREFIX "${METAHOOKSV_INSTALL_PREFIX}" CACHE PATH "Shared install prefix for all components" FORCE)
    unset(METAHOOKSV_INSTALL_PREFIX CACHE)
endif()

# Group master switches; a component builds only when both its group and its own
# option are ON.
option(METAHOOKSV_BUILD_METAHOOK "Build the MetaHook core component" ON)
option(METAHOOKSV_BUILD_PLUGINS "Build the Plugins/* components" ON)
option(METAHOOKSV_BUILD_PLUGINLIBS "Build the PluginLibs/* components" ON)
option(METAHOOKSV_BUILD_TOOLSRC "Build the CMake-based toolsrc/* components" ON)

# Also build the BulletPhysics/Renderer AVX2 variants (installed as *_AVX2.dll) for
# releases, so an existing AVX2-capable installation does not keep loading a stale
# copy probed ahead of the freshly installed baseline DLL.
option(METAHOOKSV_BUILD_AVX2 "Also build the BulletPhysics/Renderer AVX2 plugin variants" OFF)

include(cmake/LaunchGame.cmake)

# The compiler-free format entry point uses the same component registry.

# Register a per-component option (default ON) for every entry.
foreach(_def IN LISTS METAHOOKSV_COMPONENT_DEFS)
    string(REPLACE "|" ";" _fields "${_def}")
    list(GET _fields 0 _path)
    list(GET _fields 2 _opt)
    # The optional fifth field overrides the default (e.g. 0 keeps the component
    # out of the aggregate unless explicitly enabled from the cache/command line).
    list(LENGTH _fields _field_count)
    if(_field_count GREATER 4)
        list(GET _fields 4 _default)
    else()
        set(_default ON)
    endif()
    option(${_opt} "Build ${_path}" ${_default})
endforeach()

# ---------------------------------------------------------------------------
# Submodules
# ---------------------------------------------------------------------------
if(METAHOOKSV_INIT_SUBMODULES)
    find_package(Git REQUIRED)
    message(STATUS "Initializing submodules (recursive)...")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" submodule update --init --recursive
        RESULT_VARIABLE _submodule_result)
    if(NOT _submodule_result EQUAL 0)
        message(FATAL_ERROR "git submodule update --init --recursive failed (${_submodule_result}).")
    endif()
endif()

# Formatting uses the shared submodule and does not depend on native build options.
include(cmake/FormatValidation.cmake)

# ---------------------------------------------------------------------------
# Shared VC-LTL
# ---------------------------------------------------------------------------
# Every Windows component links the same VC-LTL 5.3.1 package. Resolve it once
# here (mirroring a component's cmake/Dependencies.cmake + cmake/VCLTL.cmake),
# place it under the repository-level thirdparty/, and expose VC_LTL_Root to
# each component. A component's *_prepare_vcltl() then sees the verified package
# and returns early instead of downloading its own copy.
option(METAHOOKSV_SHARED_VCLTL "Resolve one shared VC-LTL package under thirdparty/ for all components" ON)

function(metahooksv_prepare_vcltl cache_dir)
    set(version "5.3.1")
    set(expected_hash "7a18799ed3aa84a225610a5447a56bc534c5c98ccb8dec05caba0e3f633431ad")
    set(vcltl_root "${cache_dir}/VC-LTL-${version}")
    set(marker "${vcltl_root}/.verified-sha256")
    file(MAKE_DIRECTORY "${cache_dir}")
    # Debug and Release configurations share the package cache.
    file(LOCK "${cache_dir}/VC-LTL.lock" GUARD FUNCTION TIMEOUT 300)
    if(EXISTS "${marker}" AND EXISTS "${vcltl_root}/VC-LTL helper for cmake.cmake"
        AND EXISTS "${vcltl_root}/TargetPlatform/6.0.6000.0/lib/Win32/libucrt.lib")
        file(READ "${marker}" verified_hash)
        string(STRIP "${verified_hash}" verified_hash)
        if(verified_hash STREQUAL expected_hash)
            message(STATUS "Shared VC-LTL ${version} is ready: ${vcltl_root}")
            set(METAHOOKSV_VC_LTL_ROOT "${vcltl_root}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(archive "${cache_dir}/VC-LTL-${version}.7z")
    if(EXISTS "${archive}")
        file(SHA256 "${archive}" actual_hash)
        if(NOT actual_hash STREQUAL expected_hash)
            message(FATAL_ERROR "VC-LTL archive failed SHA-256 verification: ${archive}. Remove it and configure again.")
        endif()
    else()
        message(STATUS "Downloading shared VC-LTL ${version}...")
        set(download "${archive}.download")
        file(DOWNLOAD
            "https://github.com/Chuyu-Team/VC-LTL5/releases/download/v${version}/VC-LTL-Binary.7z"
            "${download}" EXPECTED_HASH "SHA256=${expected_hash}"
            TLS_VERIFY ON STATUS status SHOW_PROGRESS TIMEOUT 300 INACTIVITY_TIMEOUT 60)
        list(GET status 0 result)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "VC-LTL download failed: ${status}")
        endif()
        file(RENAME "${download}" "${archive}")
    endif()
    file(MAKE_DIRECTORY "${vcltl_root}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${vcltl_root}")
    if(NOT EXISTS "${vcltl_root}/VC-LTL helper for cmake.cmake"
        OR NOT EXISTS "${vcltl_root}/TargetPlatform/6.0.6000.0/lib/Win32/libucrt.lib")
        message(FATAL_ERROR "VC-LTL extraction is incomplete: ${vcltl_root}")
    endif()
    file(WRITE "${marker}" "${expected_hash}\n")
    message(STATUS "Shared VC-LTL ${version} prepared: ${vcltl_root}")
    set(METAHOOKSV_VC_LTL_ROOT "${vcltl_root}" PARENT_SCOPE)
endfunction()

if(METAHOOKSV_SHARED_VCLTL)
    metahooksv_prepare_vcltl("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
    # Components only provide a default for VC_LTL_Root, so this entry wins.
    set(VC_LTL_Root "${METAHOOKSV_VC_LTL_ROOT}" CACHE PATH "VC-LTL binary package root" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependency injection: map a component's external dependency variable to the
# aggregated sibling submodule path.
# ---------------------------------------------------------------------------
function(metahooksv_dependency_path dep_var out_var)
    if(dep_var STREQUAL "FORMAT_VALIDATION_SOURCE_PATH")
        set(_path "${FORMAT_VALIDATION_SOURCE_PATH}")
    elseif(dep_var STREQUAL "METAHOOK_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/MetaHook")
    elseif(dep_var STREQUAL "VGUI2EXTENSION_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VGUI2Extension")
    elseif(dep_var STREQUAL "UTILTHREADTASK_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/PluginLibs/UtilThreadTask")
    elseif(dep_var STREQUAL "UTILHTTPCLIENT_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/PluginLibs/UtilHTTPClient_libcurl")
    elseif(dep_var STREQUAL "SCOPEEXIT_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ScopeExit")
    elseif(dep_var STREQUAL "FREEIMAGE_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FreeImage_clone")
    elseif(dep_var STREQUAL "LIBXML2_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libxml2")
    elseif(dep_var STREQUAL "GLEW_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew_fork")
    elseif(dep_var STREQUAL "STEAMAPIBRIDGE_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/PluginLibs/SteamAPIBridge")
    elseif(dep_var STREQUAL "STEAMSDK_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SteamSDK")
    elseif(dep_var STREQUAL "TINYOBJLOADER_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tinyobjloader")
    elseif(dep_var STREQUAL "BULLET3_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/bullet3")
    elseif(dep_var STREQUAL "CURL_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/curl")
    elseif(dep_var STREQUAL "RAPIDJSON_SOURCE_PATH")
        # Reuse MetaHook's own rapidjson submodule (same pin) instead of a
        # second copy under the aggregator's thirdparty/.
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/MetaHook/thirdparty/rapidjson")
    elseif(dep_var STREQUAL "UTILASSETSINTEGRITY_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/PluginLibs/UtilAssetsIntegrity")
    elseif(dep_var STREQUAL "GLFW_SOURCE_PATH")
        # Test-only dependency shared by the Renderer and SteamScreenshots tests.
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw")
    elseif(dep_var STREQUAL "CHOCOBO1HASH_SOURCE_PATH")
        # Reuse MetaHook's own Chocobo1Hash submodule instead of keeping a
        # second copy under the aggregator's thirdparty/.
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/MetaHook/thirdparty/Chocobo1Hash")
    elseif(dep_var STREQUAL "SDL2_INCLUDE_DIRS")
        # SDL consumers read the public headers straight from MetaHook's SDL
        # submodules; they exist at configure time, unlike the installed copies.
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/MetaHook/thirdparty/sdl2-compat-fork/include")
    elseif(dep_var STREQUAL "SDL3_INCLUDE_DIRS")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/MetaHook/thirdparty/SDL3_fork/include")
    else()
        message(FATAL_ERROR "Unknown injected dependency variable: ${dep_var}")
    endif()
    set(${out_var} "${_path}" PARENT_SCOPE)
endfunction()

# Add one component from its own function scope: the injected dependency
# variables are normal variables of this call, so they reach only that
# component's directory and shadow its cache defaults (CMP0126). Locals are
# '_'-prefixed because the component's directory inherits this scope too.
function(metahooksv_add_component _component_path _component_deps _component_folder)
    if(NOT _component_deps STREQUAL "-")
        string(REPLACE "," ";" _dep_list "${_component_deps}")
        foreach(_dep IN LISTS _dep_list)
            metahooksv_dependency_path("${_dep}" _dep_path)
            if(NOT IS_DIRECTORY "${_dep_path}")
                message(FATAL_ERROR "${_component_path} requires ${_dep} at ${_dep_path}, which is missing. "
                    "Initialize submodules (METAHOOKSV_INIT_SUBMODULES=ON) or update them.")
            endif()
            set(${_dep} "${_dep_path}")
        endforeach()
    endif()
    # Forward the AVX2 variant switches as normal variables of this per-component
    # scope: they reach that component's directory without touching the cache, so
    # only the intended components build the extra targets.
    if(METAHOOKSV_BUILD_AVX2)
        if(_component_path STREQUAL "Plugins/BulletPhysics")
            set(BULLETPHYSICS_BUILD_AVX2 ON)
        elseif(_component_path STREQUAL "Plugins/Renderer")
            set(RENDERER_BUILD_AVX2 ON)
        endif()
    endif()

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${_component_path}" "${CMAKE_CURRENT_BINARY_DIR}/${_component_path}")

    # Group a component's own targets into one IDE filter. Vendored libraries keep
    # the folder their component assigned them, so an existing FOLDER is left as is.
    if(NOT _component_folder STREQUAL "-")
        get_property(_component_targets DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_component_path}"
            PROPERTY BUILDSYSTEM_TARGETS)
        foreach(_component_target IN LISTS _component_targets)
            get_target_property(_existing_folder "${_component_target}" FOLDER)
            if(NOT _existing_folder OR _existing_folder STREQUAL "_existing_folder-NOTFOUND")
                set_target_properties("${_component_target}" PROPERTIES FOLDER "${_component_folder}")
            endif()
        endforeach()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Build configuration shared by every component.
# ---------------------------------------------------------------------------
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported configurations" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Configuration for single-config generators" FORCE)
    endif()
    if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
        message(FATAL_ERROR "Select Debug or Release (got CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}').")
    endif()
endif()

# Component tests (opt-in per component) are reachable from this tree's CTest.
enable_testing()

# ---------------------------------------------------------------------------
# Add every enabled component.
# ---------------------------------------------------------------------------
set(_targets_metahook)
set(_targets_plugins)
set(_targets_pluginlibs)
set(_targets_toolsrc)

if((METAHOOKSV_BUILD_PLUGINS AND (METAHOOKSV_BUILD_PLUGIN_STEAMSCREENSHOTS OR METAHOOKSV_BUILD_PLUGIN_BETTERSPRAY))
    OR (METAHOOKSV_BUILD_PLUGINLIBS AND METAHOOKSV_BUILD_PLUGINLIB_UTILHTTPCLIENT_STEAMAPI))
    if(NOT METAHOOKSV_BUILD_PLUGINLIBS OR NOT METAHOOKSV_BUILD_PLUGINLIB_STEAMAPIBRIDGE)
        message(FATAL_ERROR "Enabled SteamAPI consumers require METAHOOKSV_BUILD_PLUGINLIBS and METAHOOKSV_BUILD_PLUGINLIB_STEAMAPIBRIDGE.")
    endif()
endif()

foreach(_def IN LISTS METAHOOKSV_COMPONENT_DEFS)
    string(REPLACE "|" ";" _fields "${_def}")
    list(GET _fields 0 _path)
    list(GET _fields 1 _group)
    list(GET _fields 2 _opt)
    list(GET _fields 3 _dep_vars)

    # Group master switch.
    if(_group STREQUAL "metahook")
        set(_group_enabled ${METAHOOKSV_BUILD_METAHOOK})
    elseif(_group STREQUAL "plugins")
        set(_group_enabled ${METAHOOKSV_BUILD_PLUGINS})
    elseif(_group STREQUAL "pluginlibs")
        set(_group_enabled ${METAHOOKSV_BUILD_PLUGINLIBS})
    elseif(_group STREQUAL "toolsrc")
        set(_group_enabled ${METAHOOKSV_BUILD_TOOLSRC})
    else()
        set(_group_enabled ON)
    endif()

    if(NOT _group_enabled OR NOT ${_opt})
        message(STATUS "Skipping ${_path} (disabled)")
        continue()
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_path}/CMakeLists.txt")
        message(FATAL_ERROR "${_path} has no CMakeLists.txt at ${CMAKE_CURRENT_SOURCE_DIR}/${_path}. "
            "Initialize submodules (METAHOOKSV_INIT_SUBMODULES=ON) or update them.")
    endif()

    # Each component's primary target is named after its directory.
    get_filename_component(_name "${_path}" NAME)
    message(STATUS "Enabling ${_path} (target '${_name}')")
    # Each group maps to one IDE filter; MetaHook keeps its own.
    if(_group STREQUAL "metahook")
        set(_component_folder "MetaHookCore")
    elseif(_group STREQUAL "plugins")
        set(_component_folder "MetaHookPlugins")
    elseif(_group STREQUAL "pluginlibs")
        set(_component_folder "MetaHookPluginLibs")
    elseif(_group STREQUAL "toolsrc")
        set(_component_folder "MetaHookTools")
    else()
        set(_component_folder "-")
    endif()
    metahooksv_add_component("${_path}" "${_dep_vars}" "${_component_folder}")
    if(NOT TARGET ${_name})
        message(FATAL_ERROR "${_path} did not define its primary target '${_name}'.")
    endif()

    # Plugin lists and runtime library lookups use the same names in both configs.
    # In particular, do not inherit Bullet's cached CMAKE_DEBUG_POSTFIX (_Debug).
    if(_group STREQUAL "plugins" OR _group STREQUAL "pluginlibs")
        set_target_properties(${_name} PROPERTIES DEBUG_POSTFIX "")
    endif()

    list(APPEND _targets_${_group} ${_name})
endforeach()

# ---------------------------------------------------------------------------
# Group convenience targets. MetaHook needs none: its own target is `MetaHook`,
# and a `metahook` group target would differ from it only by case, which
# Windows file names and Visual Studio project names cannot tell apart.
# ---------------------------------------------------------------------------
install(DIRECTORY assets/ DESTINATION .)

foreach(_group_target IN ITEMS plugins pluginlibs tools)
    add_custom_target(${_group_target})
endforeach()
if(_targets_plugins)
    add_dependencies(plugins ${_targets_plugins})
endif()
if(_targets_pluginlibs)
    add_dependencies(pluginlibs ${_targets_pluginlibs})
endif()
if(_targets_toolsrc)
    add_dependencies(tools ${_targets_toolsrc})
endif()
add_custom_target(all-components)
add_dependencies(all-components plugins pluginlibs tools ${_targets_metahook})
# MetaHook's blob companion is not a component primary target, so add it here.
if(TARGET MetaHook_blob)
    add_dependencies(all-components MetaHook_blob)
endif()

# toolsrc/BSPLocalizationTools (.NET 8) and toolsrc/MetahookInstaller (.NET 10) are
# .NET projects driven by their own solution files; they are intentionally not
# aggregated here.
if(METAHOOKSV_BUILD_TOOLSRC)
    message(STATUS "Note: toolsrc/BSPLocalizationTools and toolsrc/MetahookInstaller are .NET "
        "projects; build them via their own solution (dotnet build).")
endif()

if(METAHOOKSV_ENABLE_LAUNCH_GAME)
    metahooksv_add_launch_game()
endif()

message(STATUS "MetaHookSv aggregator configured.")
message(STATUS "  Components: metahook=${_targets_metahook} plugins=${_targets_plugins} pluginlibs=${_targets_pluginlibs} tools=${_targets_toolsrc}")
message(STATUS "  Layout: ${METAHOOKSV_BUILD_ARCH}/${METAHOOKSV_BUILD_CONFIG}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
