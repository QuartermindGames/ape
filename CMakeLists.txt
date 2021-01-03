# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(Yin)

message("Configuring Yin...")

# Set all of our output directories.


include_directories(src/)
include_directories(src/core/include/)

include(DetermineTargetArchitecture.cmake)
determine_target_architecture(YIN_SYSTEM_ARCH)

if (NOT UNIX)
    message("Not Unix, assuming Win32 - " ${YIN_SYSTEM_ARCH})
    if (${YIN_SYSTEM_ARCH} MATCHES x86_64)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x64/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x64/")
    else ()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x86/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x86/")
    endif ()
elseif(NOT APPLE) # assume linux
    message("Not Win32, assuming Linux - " ${YIN_SYSTEM_ARCH})
    if (${YIN_SYSTEM_ARCH} MATCHES x86_64)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x64/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x64/")
    else ()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x86/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x86/")
    endif ()
endif ()

add_subdirectory(src/3rdparty/platform/platform/)
add_subdirectory(src/launcher_sdl2/)
add_subdirectory(src/core/)
add_subdirectory(src/engine/)

# tools
add_subdirectory(src/editor/)
add_subdirectory(src/tools/qmap2world/)
