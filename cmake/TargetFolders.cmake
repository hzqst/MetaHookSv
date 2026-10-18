# IDE solution-folder helpers for the aggregate solution.
#
# The Visual Studio generator derives its solution-folder hierarchy from each
# target's FOLDER property: "A/B" places the target in folder B nested under
# folder A. Components are added from their own source directories, so the
# aggregator assigns these folders only after add_subdirectory() has returned
# and every component target exists.

# Collect the build-system targets created in _component_root and in every
# source directory it added below itself. SUBDIRECTORIES is used instead of a
# filesystem walk: only directories actually configured by the component are
# visited, so an add_subdirectory() that a component skipped (for example the
# shared "if(NOT TARGET glfw)" guard) never appears.
#
# Directories outside the component's source tree are ignored. Vendored
# dependencies (GLEW, Bullet3, FreeImage, GLFW, libxml2, curl, ...) are added
# from the repository-level thirdparty/ tree, so their targets keep the FOLDER
# their component assigned them. IS_PREFIX compares path components, so a
# sibling directory such as "RendererExtra" never matches "Renderer".
function(metahooksv_collect_component_targets _component_root _out_var)
    set(_pending "${_component_root}")
    set(_targets "")
    while(_pending)
        list(POP_FRONT _pending _directory)
        get_property(_directory_targets DIRECTORY "${_directory}"
            PROPERTY BUILDSYSTEM_TARGETS)
        list(APPEND _targets ${_directory_targets})
        get_property(_subdirectories DIRECTORY "${_directory}"
            PROPERTY SUBDIRECTORIES)
        foreach(_subdirectory IN LISTS _subdirectories)
            cmake_path(IS_PREFIX _component_root "${_subdirectory}"
                NORMALIZE _subdirectory_is_component_local)
            if(_subdirectory_is_component_local)
                list(APPEND _pending "${_subdirectory}")
            endif()
        endforeach()
    endwhile()
    if(_targets)
        list(REMOVE_DUPLICATES _targets)
    endif()
    set(${_out_var} "${_targets}" PARENT_SCOPE)
endfunction()

# Give a component's targets its IDE filter. An existing FOLDER wins: the
# component already knows where its vendored libraries and the shared
# FormatValidation targets belong. The one exception is the group root itself:
# BulletPhysics, BulletPhysics_AVX2 and Renderer_AVX2 set FOLDER
# "MetaHookPlugins" in their own CMakeLists, and in the aggregate that must
# become "MetaHookPlugins/<component>" instead of staying flat at the root.
function(metahooksv_assign_component_folders _targets _group_folder _component_folder)
    foreach(_target IN LISTS _targets)
        get_target_property(_existing_folder "${_target}" FOLDER)
        if(NOT _existing_folder
            OR _existing_folder STREQUAL "_existing_folder-NOTFOUND"
            OR _existing_folder STREQUAL _group_folder)
            set_target_properties("${_target}" PROPERTIES FOLDER "${_component_folder}")
        endif()
    endforeach()
endfunction()
