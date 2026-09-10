cmake_minimum_required(VERSION 3.20)
project(SteamScreenshotsCaptureTests LANGUAGES C CXX)

get_filename_component(REPOSITORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${REPOSITORY_ROOT}/thirdparty/glfw" "glfw" EXCLUDE_FROM_ALL)

set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ONLY_LIBS ON CACHE BOOL "" FORCE)
add_subdirectory("${REPOSITORY_ROOT}/thirdparty/glew_fork" "glew" EXCLUDE_FROM_ALL)

add_executable(capture_tests capture_tests.cpp ../gl_capture.cpp)
target_compile_features(capture_tests PRIVATE cxx_std_20)
set_target_properties(capture_tests PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(capture_tests PRIVATE GLEW_STATIC GLFW_INCLUDE_NONE)
target_include_directories(capture_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
    "${REPOSITORY_ROOT}/thirdparty/glew_fork/include/GL")
target_link_libraries(capture_tests PRIVATE glfw libglew_static)

enable_testing()
foreach(test_case IN ITEMS sync-default async-default sync-dirty async-dirty
        wait-timeout wait-failure framebuffer-unavailable framebuffer-entrypoint-unavailable arb-framebuffer)
    add_test(NAME "capture.${test_case}" COMMAND capture_tests "${test_case}")
    set_tests_properties("capture.${test_case}" PROPERTIES TIMEOUT 20)
endforeach()
