# Optional Visual Studio F5 workflow. Keep all machine settings in the cache.
option(METAHOOKSV_ENABLE_LAUNCH_GAME "Add the Visual Studio LaunchGame build/deploy/debug target" OFF)
set(METAHOOKSV_GAME_DIRECTORY "" CACHE PATH "Game root (empty: discover the Steam app through InstallerCLI)")
set(METAHOOKSV_GAME_APPID "225840" CACHE STRING "Steam app ID for LaunchGame")
set(METAHOOKSV_GAME_MOD "" CACHE STRING "Game mod directory (empty: InstallerCLI's app default)")
set(METAHOOKSV_GAME_ARGUMENTS "" CACHE STRING "Extra game arguments after -insecure -game <mod>")

if(METAHOOKSV_ENABLE_LAUNCH_GAME)
    if(NOT CMAKE_GENERATOR MATCHES "^Visual Studio ")
        message(FATAL_ERROR "LaunchGame requires a Visual Studio generator with -A Win32.")
    endif()
    if(NOT METAHOOKSV_BUILD_METAHOOK)
        message(FATAL_ERROR "LaunchGame requires METAHOOKSV_BUILD_METAHOOK=ON (both launchers).")
    endif()
    find_program(METAHOOKSV_DOTNET_EXECUTABLE NAMES dotnet REQUIRED)
endif()

function(metahooksv_add_launch_game)
    set(_launch_root "${CMAKE_BINARY_DIR}/launch-game")
    set(_cli_root "${CMAKE_SOURCE_DIR}/toolsrc/MetahookInstaller/src")
    set(_cli_project "${_cli_root}/MetahookInstallerCLI/MetahookInstallerCLI.csproj")
    if(NOT EXISTS "${_cli_project}")
        message(FATAL_ERROR "LaunchGame requires the toolsrc/MetahookInstaller submodule.")
    endif()

    # A generated VS project needs its debugger command at configure time.
    # Build only the CLI/Core projects, never the GUI or the native solution.
    file(MAKE_DIRECTORY "${_launch_root}/configure")
    execute_process(
        COMMAND "${METAHOOKSV_DOTNET_EXECUTABLE}" build "${_cli_project}"
            --configuration Release --output "${_launch_root}/configure" --nologo
        RESULT_VARIABLE _result OUTPUT_VARIABLE _output ERROR_VARIABLE _error)
    if(NOT _result STREQUAL "0")
        message(FATAL_ERROR "Cannot build InstallerCLI; install a compatible .NET SDK/runtime.\n${_output}\n${_error}")
    endif()

    set(_query_args -appid "${METAHOOKSV_GAME_APPID}")
    if(NOT METAHOOKSV_GAME_DIRECTORY STREQUAL "")
        list(APPEND _query_args -gamedir "${METAHOOKSV_GAME_DIRECTORY}")
    endif()
    if(NOT METAHOOKSV_GAME_MOD STREQUAL "")
        list(APPEND _query_args -moddir "${METAHOOKSV_GAME_MOD}")
    endif()
    execute_process(
        COMMAND "${METAHOOKSV_DOTNET_EXECUTABLE}" "${_launch_root}/configure/MetahookInstallerCLI.dll"
            ${_query_args} -describe-target
        WORKING_DIRECTORY "${_launch_root}/configure"
        RESULT_VARIABLE _result OUTPUT_VARIABLE _description ERROR_VARIABLE _error)
    if(NOT _result STREQUAL "0")
        message(FATAL_ERROR "Cannot resolve LaunchGame target. Set METAHOOKSV_GAME_DIRECTORY or check METAHOOKSV_GAME_APPID/MOD.\n${_error}")
    endif()
    string(JSON _game_directory GET "${_description}" GameDirectory)
    string(JSON _game_mod GET "${_description}" ModDirectory)
    string(JSON _launcher GET "${_description}" LauncherPath)

    # Reconfigure when the query implementation changes. Ignore generated bin/obj.
    file(GLOB_RECURSE _cli_inputs CONFIGURE_DEPENDS
        "${_cli_root}/MetahookInstallerCLI/*.cs" "${_cli_root}/MetahookInstallerCLI/*.csproj"
        "${_cli_root}/MetahookInstallerCore/*.cs" "${_cli_root}/MetahookInstallerCore/*.csproj")
    list(FILTER _cli_inputs EXCLUDE REGEX "/(bin|obj)/")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_cli_inputs})

    # Bracket-quoted settings keep Windows paths and argument quoting intact.
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LaunchGameSettings.cmake.in"
        "${_launch_root}/settings.cmake" @ONLY)
    file(CONFIGURE OUTPUT "${_launch_root}/dummy_launcher.cpp"
        CONTENT "int main() { return 0; }\n" @ONLY)
    add_executable(LaunchGame EXCLUDE_FROM_ALL "${_launch_root}/dummy_launcher.cpp")
    set(_arguments "-insecure -game \"${_game_mod}\"")
    if(NOT METAHOOKSV_GAME_ARGUMENTS STREQUAL "")
        string(APPEND _arguments " ${METAHOOKSV_GAME_ARGUMENTS}")
    endif()
    set_target_properties(LaunchGame PROPERTIES
        FOLDER "Launch-debugging"
        VS_DEBUGGER_COMMAND "${_launcher}"
        VS_DEBUGGER_WORKING_DIRECTORY "${_game_directory}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "${_arguments}"
        VS_GLOBAL_DebuggerFlavor WindowsLocalDebugger
        VS_GLOBAL_DebuggerType NativeOnly
        VS_GLOBAL_DisableFastUpToDateCheck true)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT LaunchGame)

    # No output stamp: every build/F5 must redeploy, even if sources are unchanged.
    add_custom_target(MetaHookSvDeployGame
        COMMAND "${CMAKE_COMMAND}" "-DSETTINGS=${_launch_root}/settings.cmake"
            "-DCONFIG=$<CONFIG>" -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/DeployGame.cmake"
        COMMENT "Installing and deploying LaunchGame ($<CONFIG>)"
        VERBATIM USES_TERMINAL)
    set_target_properties(MetaHookSvDeployGame PROPERTIES
        FOLDER "Launch-debugging"
        VS_GLOBAL_DisableFastUpToDateCheck true)
    add_dependencies(MetaHookSvDeployGame all-components)
    # SDL runtime libraries are installed via install(FILES), not linked by MetaHook.
    foreach(_runtime IN ITEMS SDL2 SDL3-shared)
        if(TARGET ${_runtime})
            add_dependencies(MetaHookSvDeployGame ${_runtime})
        endif()
    endforeach()
    add_dependencies(LaunchGame MetaHookSvDeployGame)
    message(STATUS "LaunchGame debugger: ${_launcher} ${_arguments}")
endfunction()
