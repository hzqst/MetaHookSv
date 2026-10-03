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

    "Plugins/BulletPhysics|plugins|METAHOOKSV_BUILD_PLUGIN_BULLETPHYSICS|METAHOOK_SOURCE_PATH,VGUI2EXTENSION_SOURCE_PATH"
    "Plugins/CaptionMod|plugins|METAHOOKSV_BUILD_PLUGIN_CAPTIONMOD|METAHOOK_SOURCE_PATH,VGUI2EXTENSION_SOURCE_PATH"
    "Plugins/HeapPatch|plugins|METAHOOKSV_BUILD_PLUGIN_HEAPPATCH|METAHOOK_SOURCE_PATH"
    "Plugins/PrecacheManager|plugins|METAHOOKSV_BUILD_PLUGIN_PRECACHEMANAGER|METAHOOK_SOURCE_PATH"
    "Plugins/Renderer|plugins|METAHOOKSV_BUILD_PLUGIN_RENDERER|METAHOOK_SOURCE_PATH,VGUI2EXTENSION_SOURCE_PATH,UTILTHREADTASK_SOURCE_PATH"
    "Plugins/ResourceReplacer|plugins|METAHOOKSV_BUILD_PLUGIN_RESOURCEREPLACER|METAHOOK_SOURCE_PATH"
    "Plugins/SCCameraFix|plugins|METAHOOKSV_BUILD_PLUGIN_SCCAMERAFIX|METAHOOK_SOURCE_PATH"
    "Plugins/SCModelDownloader|plugins|METAHOOKSV_BUILD_PLUGIN_SCMODELDOWNLOADER|-"
    "Plugins/SteamScreenshots|plugins|METAHOOKSV_BUILD_PLUGIN_STEAMSCREENSHOTS|-"
    "Plugins/StudioEvents|plugins|METAHOOKSV_BUILD_PLUGIN_STUDIOEVENTS|METAHOOK_SOURCE_PATH"
    "Plugins/ThreadGuard|plugins|METAHOOKSV_BUILD_PLUGIN_THREADGUARD|METAHOOK_SOURCE_PATH"
    "Plugins/VGUI2Extension|plugins|METAHOOKSV_BUILD_PLUGIN_VGUI2EXTENSION|METAHOOK_SOURCE_PATH"

    "PluginLibs/UtilAssetsIntegrity|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILASSETSINTEGRITY|METAHOOK_SOURCE_PATH"
    "PluginLibs/UtilHTTPClient_SteamAPI|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILHTTPCLIENT_STEAMAPI|METAHOOK_SOURCE_PATH"
    "PluginLibs/UtilHTTPClient_libcurl|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILHTTPCLIENT_LIBCURL|METAHOOK_SOURCE_PATH"
    "PluginLibs/UtilThreadTask|pluginlibs|METAHOOKSV_BUILD_PLUGINLIB_UTILTHREADTASK|METAHOOK_SOURCE_PATH"

    "toolsrc/SteamAppsLocation|toolsrc|METAHOOKSV_BUILD_TOOL_STEAMAPPSLOCATION|-"
)

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
            if(NOT EXISTS "${_dep_path}/CMakeLists.txt")
                message(FATAL_ERROR "${_name} requires ${_dep} at ${_dep_path}, which is missing.")
            endif()
            list(APPEND _dep_args "-D${_dep}=${_dep_path}")
        endforeach()
    endif()

    if(METAHOOKSV_INSTALL)
        set(_install_args "")
    else()
        set(_install_args INSTALL_COMMAND "")
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
            ${_dep_args}
        ${_install_args}
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
