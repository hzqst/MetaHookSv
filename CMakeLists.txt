# MetaHookSv aggregator (superbuild)
#
# This repository aggregates the MetaHookSv components, each of which lives in
# its own git submodule and is a self-contained CMake project:
#
#   MetaHook/          core launcher / SDK host
#   Plugins/*          game plugins
#   PluginLibs/*       shared plugin libraries
#   toolsrc/*          standalone tools (CMake-based ones only)
#
# Why a superbuild and not add_subdirectory():
#   Several components configure the *same* dependency targets (Renderer and
#   UtilAssetsIntegrity both add_subdirectory() FreeImage; BulletPhysics and
#   Renderer both add GLEW). Placing them in one CMake tree would collide on the
#   FreeImage / libglew_static target names and fail. Configuring each component
#   in its own binary directory keeps those private dependency trees isolated.
#
# Each component is pinned to a submodule revision, so this aggregate is a fixed
# set of per-component builds that install into one shared prefix.
#
# Usage (MSVC x86 / Win32 is required by every component):
#   cmake -S . -B build -A Win32
#   cmake --build build --config Release
#   cmake --install build --config Release      # stages into build/output
#
# The aggregated sibling sources are injected so components consume the local
# submodules instead of downloading their own copies:
#   METAHOOK_SOURCE_PATH          -> MetaHook/
#   VGUI2EXTENSION_SOURCE_PATH    -> Plugins/VGUI2Extension/
#   UTILTHREADTASK_SOURCE_PATH    -> PluginLibs/UtilThreadTask/

cmake_minimum_required(VERSION 3.21)

project(MetaHookSv LANGUAGES NONE)

include(ExternalProject)

if(NOT WIN32)
    message(FATAL_ERROR "MetaHookSv components require Windows. Configure on Windows with MSVC x86 (-A Win32).")
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(METAHOOKSV_INIT_SUBMODULES "Run 'git submodule update --init --recursive' at configure time" ON)
option(METAHOOKSV_INSTALL "Run each component's install step into a shared prefix" ON)
set(METAHOOKSV_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/output" CACHE PATH "Shared install prefix for all components")

# Group master switches; a component builds only when both its group and its own
# option are ON.
option(METAHOOKSV_BUILD_METAHOOK "Build the MetaHook core component" ON)
option(METAHOOKSV_BUILD_PLUGINS "Build the Plugins/* components" ON)
option(METAHOOKSV_BUILD_PLUGINLIBS "Build the PluginLibs/* components" ON)
option(METAHOOKSV_BUILD_TOOLSRC "Build the CMake-based toolsrc/* components" ON)

# Component registry: <path>|<group>|<per-component option>|<injected dep vars>
# Dependency vars are comma-separated; '-' means the component fetches everything
# itself. (';' cannot be used: it is a CMake list separator even inside quotes.)
set(METAHOOKSV_COMPONENT_DEFS
    "MetaHook|metahook|METAHOOKSV_BUILD_METAHOOK|-"

    "Plugins/BulletPhysics|plugins|METAHOOKSV_BUILD_PLUGIN_BULLETPHYSICS|METAHOOK_SOURCE_PATH,VGUI2EXTENSION_SOURCE_PATH,SCOPEEXIT_SOURCE_PATH,GLEW_SOURCE_PATH,TINYOBJLOADER_SOURCE_PATH,CHOCOBO1HASH_SOURCE_PATH"
    "Plugins/CaptionMod|plugins|METAHOOKSV_BUILD_PLUGIN_CAPTIONMOD|METAHOOK_SOURCE_PATH,VGUI2EXTENSION_SOURCE_PATH"
    "Plugins/HeapPatch|plugins|METAHOOKSV_BUILD_PLUGIN_HEAPPATCH|METAHOOK_SOURCE_PATH"
    "Plugins/PrecacheManager|plugins|METAHOOKSV_BUILD_PLUGIN_PRECACHEMANAGER|METAHOOK_SOURCE_PATH"
    "Plugins/Renderer|plugins|METAHOOKSV_BUILD_PLUGIN_RENDERER|METAHOOK_SOURCE_PATH,VGUI2EXTENSION_SOURCE_PATH,UTILTHREADTASK_SOURCE_PATH,SCOPEEXIT_SOURCE_PATH,GLEW_SOURCE_PATH,FREEIMAGE_SOURCE_PATH,TINYOBJLOADER_SOURCE_PATH"
    "Plugins/ResourceReplacer|plugins|METAHOOKSV_BUILD_PLUGIN_RESOURCEREPLACER|METAHOOK_SOURCE_PATH"
    "Plugins/SCCameraFix|plugins|METAHOOKSV_BUILD_PLUGIN_SCCAMERAFIX|METAHOOK_SOURCE_PATH"
    "Plugins/SCModelDownloader|plugins|METAHOOKSV_BUILD_PLUGIN_SCMODELDOWNLOADER|METAHOOK_SOURCE_PATH,VGUI2EXTENSION_SOURCE_PATH,SCOPEEXIT_SOURCE_PATH"
    "Plugins/SteamScreenshots|plugins|METAHOOKSV_BUILD_PLUGIN_STEAMSCREENSHOTS|STEAMSDK_SOURCE_PATH,GLEW_SOURCE_PATH"
    "Plugins/StudioEvents|plugins|METAHOOKSV_BUILD_PLUGIN_STUDIOEVENTS|METAHOOK_SOURCE_PATH"
    "Plugins/ThreadGuard|plugins|METAHOOKSV_BUILD_PLUGIN_THREADGUARD|METAHOOK_SOURCE_PATH"
    "Plugins/VGUI2Extension|plugins|METAHOOKSV_BUILD_PLUGIN_VGUI2EXTENSION|METAHOOK_SOURCE_PATH"

    "PluginLibs/UtilAssetsIntegrity|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILASSETSINTEGRITY|METAHOOK_SOURCE_PATH,SCOPEEXIT_SOURCE_PATH,FREEIMAGE_SOURCE_PATH"
    "PluginLibs/UtilHTTPClient_SteamAPI|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILHTTPCLIENT_STEAMAPI|METAHOOK_SOURCE_PATH,SCOPEEXIT_SOURCE_PATH,STEAMSDK_SOURCE_PATH"
    "PluginLibs/UtilHTTPClient_libcurl|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILHTTPCLIENT_LIBCURL|METAHOOK_SOURCE_PATH,SCOPEEXIT_SOURCE_PATH"
    "PluginLibs/UtilThreadTask|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILTHREADTASK|METAHOOK_SOURCE_PATH"

    "toolsrc/SteamAppsLocation|toolsrc|METAHOOKSV_BUILD_TOOL_STEAMAPPSLOCATION|-"
)

# Components that consume SDL2/SDL3 headers. They are pointed at the SDL headers
# that MetaHook builds and installs into the shared prefix (Renderer and
# VGUI2Extension deliberately do not build SDL themselves).
set(METAHOOKSV_SDL_CONSUMERS Renderer VGUI2Extension)

# Register a per-component option (default ON) for every entry.
foreach(_def IN LISTS METAHOOKSV_COMPONENT_DEFS)
    string(REPLACE "|" ";" _fields "${_def}")
    list(GET _fields 0 _path)
    list(GET _fields 2 _opt)
    option(${_opt} "Build ${_path}" ON)
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

# ---------------------------------------------------------------------------
# Shared VC-LTL
# ---------------------------------------------------------------------------
# Every Windows component links the same VC-LTL 5.3.1 package. Resolve it once
# here (mirroring a component's cmake/Dependencies.cmake + cmake/VCLTL.cmake),
# place it under the repository-level thirdparty/, and pass VC_LTL_Root to each
# component. A component's *_prepare_vcltl() then sees the verified package and
# returns early instead of downloading its own copy.
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

set(METAHOOKSV_VC_LTL_ROOT "")
if(METAHOOKSV_SHARED_VCLTL)
    metahooksv_prepare_vcltl("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
endif()

# ---------------------------------------------------------------------------
# Dependency injection: map a component's external dependency variable to the
# aggregated sibling submodule path.
# ---------------------------------------------------------------------------
function(metahooksv_dependency_path dep_var out_var)
    if(dep_var STREQUAL "METAHOOK_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/MetaHook")
    elseif(dep_var STREQUAL "VGUI2EXTENSION_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VGUI2Extension")
    elseif(dep_var STREQUAL "UTILTHREADTASK_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/PluginLibs/UtilThreadTask")
    elseif(dep_var STREQUAL "SCOPEEXIT_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ScopeExit")
    elseif(dep_var STREQUAL "FREEIMAGE_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FreeImage_clone")
    elseif(dep_var STREQUAL "GLEW_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew_fork")
    elseif(dep_var STREQUAL "STEAMSDK_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SteamSDK")
    elseif(dep_var STREQUAL "TINYOBJLOADER_SOURCE_PATH")
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tinyobjloader")
    elseif(dep_var STREQUAL "CHOCOBO1HASH_SOURCE_PATH")
        # Reuse MetaHook's own Chocobo1Hash submodule instead of keeping a
        # second copy under the aggregator's thirdparty/.
        set(_path "${CMAKE_CURRENT_SOURCE_DIR}/MetaHook/thirdparty/Chocobo1Hash")
    else()
        message(FATAL_ERROR "Unknown injected dependency variable: ${dep_var}")
    endif()
    set(${out_var} "${_path}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Build configuration passed to every component's sub-build.
# ---------------------------------------------------------------------------
# Multi-config generators (Visual Studio) select the configuration with
# `--config`; single-config generators (Ninja) need CMAKE_BUILD_TYPE.
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(_metahooksv_config_args)
if(NOT _is_multi_config)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Configuration for single-config generators" FORCE)
    endif()
    if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
        message(FATAL_ERROR "Select Debug or Release (got CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}').")
    endif()
    list(APPEND _metahooksv_config_args "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()

# ---------------------------------------------------------------------------
# Register one ExternalProject per enabled component.
# ---------------------------------------------------------------------------
set(_targets_metahook)
set(_targets_plugins)
set(_targets_pluginlibs)
set(_targets_toolsrc)

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

    set(_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_path}")
    if(NOT EXISTS "${_source_dir}/CMakeLists.txt")
        message(FATAL_ERROR "${_path} has no CMakeLists.txt at ${_source_dir}. "
            "Initialize submodules (METAHOOKSV_INIT_SUBMODULES=ON) or update them.")
    endif()

    get_filename_component(_name "${_path}" NAME)
    set(_dep_args)
    if(NOT _dep_vars STREQUAL "-")
        string(REPLACE "," ";" _dep_list "${_dep_vars}")
        foreach(_dep IN LISTS _dep_list)
            metahooksv_dependency_path("${_dep}" _dep_path)
            if(NOT IS_DIRECTORY "${_dep_path}")
                message(FATAL_ERROR "${_name} requires ${_dep} at ${_dep_path}, which is missing. "
                    "Initialize submodules (METAHOOKSV_INIT_SUBMODULES=ON) or update them.")
            endif()
            list(APPEND _dep_args "-D${_dep}=${_dep_path}")
        endforeach()
    endif()

    if(METAHOOKSV_INSTALL)
        set(_install_args "")
    else()
        set(_install_args INSTALL_COMMAND "")
    endif()

    set(_vcltl_args)
    if(METAHOOKSV_VC_LTL_ROOT)
        list(APPEND _vcltl_args "-DVC_LTL_Root=${METAHOOKSV_VC_LTL_ROOT}")
    endif()

    # SDL consumers get the headers MetaHook installs into the shared prefix and
    # must build after MetaHook (which builds/installs SDL3 and SDL2).
    set(_sdl_args)
    set(_sdl_dep)
    if(_name IN_LIST METAHOOKSV_SDL_CONSUMERS)
        set(_sdl_include "${METAHOOKSV_INSTALL_PREFIX}/include")
        list(APPEND _sdl_args
            "-DSDL3_INCLUDE_DIRS=${_sdl_include}"
            "-DSDL2_INCLUDE_DIRS=${_sdl_include}")
        if(_targets_metahook)
            set(_sdl_dep DEPENDS ${_targets_metahook})
        else()
            message(WARNING "${_name} consumes SDL but MetaHook is disabled; pass SDL2_INCLUDE_DIRS/SDL3_INCLUDE_DIRS manually.")
        endif()
    endif()

    message(STATUS "Enabling ${_path} (target '${_name}')")
    ExternalProject_Add(${_name}
        SOURCE_DIR "${_source_dir}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/build/${_name}"
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=${METAHOOKSV_INSTALL_PREFIX}"
            ${_metahooksv_config_args}
            ${_vcltl_args}
            ${_sdl_args}
            ${_dep_args}
        ${_install_args}
        ${_sdl_dep}
    )

    if(_group STREQUAL "metahook")
        list(APPEND _targets_metahook ${_name})
    elseif(_group STREQUAL "plugins")
        list(APPEND _targets_plugins ${_name})
    elseif(_group STREQUAL "pluginlibs")
        list(APPEND _targets_pluginlibs ${_name})
    elseif(_group STREQUAL "toolsrc")
        list(APPEND _targets_toolsrc ${_name})
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Group convenience targets.
# ---------------------------------------------------------------------------
add_custom_target(metahook DEPENDS ${_targets_metahook})
add_custom_target(plugins DEPENDS ${_targets_plugins})
add_custom_target(pluginlibs DEPENDS ${_targets_pluginlibs})
add_custom_target(tools DEPENDS ${_targets_toolsrc})
add_custom_target(all-components DEPENDS metahook plugins pluginlibs tools)

# toolsrc/BSPLocalizationTools and toolsrc/MetahookInstaller are .NET 8 projects
# driven by their own solution files; they are intentionally not aggregated here.
if(METAHOOKSV_BUILD_TOOLSRC)
    message(STATUS "Note: toolsrc/BSPLocalizationTools and toolsrc/MetahookInstaller are .NET "
        "projects; build them via their own .sln (dotnet build).")
endif()

message(STATUS "MetaHookSv aggregator configured.")
message(STATUS "  Components: metahook=${_targets_metahook} plugins=${_targets_plugins} pluginlibs=${_targets_pluginlibs} tools=${_targets_toolsrc}")
message(STATUS "  Install prefix: ${METAHOOKSV_INSTALL_PREFIX}")
