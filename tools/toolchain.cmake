cmake_minimum_required(VERSION 3.15) # or higher

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Get the current setting of CMP0091 and output it
get_property(policy_setting GLOBAL PROPERTY CMP0091)
message(STATUS "Policy CMP0091: ${policy_setting}")
message(STATUS "MSVC Runtime Library: ${CMAKE_MSVC_RUNTIME_LIBRARY}")