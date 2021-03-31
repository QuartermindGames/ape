# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(Yin)

message("Configuring Yin...")

# Set all of our output directories.

include_directories(src/)
include_directories(src/common/include/)

include(DetermineTargetArchitecture.cmake)
determine_target_architecture(YIN_SYSTEM_ARCH)

if (NOT UNIX)
    message("Not Unix, assuming Win32 - " ${YIN_SYSTEM_ARCH})
    if (${YIN_SYSTEM_ARCH} MATCHES x86_64)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x64/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x64/")
        link_directories(src/3rdparty/sdl2/lib/x64/)
    else ()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x86/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/win32-x86/")
        link_directories(src/3rdparty/sdl2/lib/x86/)
    endif ()
    include_directories(src/3rdparty/sdl2/include/)
elseif (NOT APPLE) # assume linux
    message("Not Win32, assuming Linux - " ${YIN_SYSTEM_ARCH})
    if (${YIN_SYSTEM_ARCH} MATCHES x86_64)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x64/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x64/")
    else ()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x86/")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release/runtime/linux-x86/")
    endif ()
endif ()

# Get the current working branch
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the latest abbreviated commit hash of the working branch
execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the number of commits on the working branch
execute_process(
        COMMAND git rev-list HEAD --count
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
add_definitions(-DGIT_COMMIT_COUNT="${GIT_COMMIT_COUNT}")
add_definitions(-DGIT_BRANCH="${GIT_BRANCH}")

# external libraries
add_subdirectory(src/3rdparty/platform/platform/)
add_subdirectory(src/3rdparty/platform/plugins/driver_opengl/)
add_subdirectory(src/3rdparty/fox-1.6.57/)

# main projects
add_subdirectory(src/launcher_sdl2/)
add_subdirectory(src/common/)
add_subdirectory(src/engine/)
add_subdirectory(src/game/)

# tools
add_subdirectory(src/pkgman/)
add_subdirectory(src/editor/)
add_subdirectory(src/ycvm/)
add_subdirectory(src/tools/qmap2world/)
