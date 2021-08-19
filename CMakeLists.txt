# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(Yin)

message("Configuring Yin...")

# Set all of our output directories.

include_directories(src/)
include_directories(src/libcommon/include/)

include(DetermineTargetArchitecture.cmake)
determine_target_architecture(YIN_SYSTEM_ARCH)

if(UNIX)
    message("Assuming Unix - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/unix-${YIN_SYSTEM_ARCH}/")
elseif(WIN32)
    message("Assuming Win32 - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/win32-${YIN_SYSTEM_ARCH}/")
elseif(APPLE)
    message("Assuming Apple - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/apple-${YIN_SYSTEM_ARCH}/")
else()
    message("Unknown platform - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/${YIN_SYSTEM_ARCH}/")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${YIN_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${YIN_OUTPUT_DIR}")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${YIN_OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${YIN_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${YIN_OUTPUT_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

if(WIN32)
    if(${YIN_SYSTEM_ARCH} MATCHES x86_64)
        link_directories(src/3rdparty/sdl2/lib/x64/)
    else()
        link_directories(src/3rdparty/sdl2/lib/x86/)
    endif()
    include_directories(src/3rdparty/sdl2/include/)
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3 -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3 -D_CRT_SECURE_NO_WARNINGS")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-variable -Wno-unused-function -Werror=implicit-function-declaration")
endif()

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
#add_subdirectory(src/3rdparty/fox-1.6.57_cmake/)

# main projects
add_subdirectory(src/platform/plcore/)
add_subdirectory(src/platform/plgraphics/)
add_subdirectory(src/platform/plmodel/)
add_subdirectory(src/platform/plugins/driver_opengl/)
add_subdirectory(src/launcher_sdl2/)
add_subdirectory(src/libcommon/)
add_subdirectory(src/engine/)
add_subdirectory(src/game/)

# tools
#add_subdirectory(src/editor/)
add_subdirectory(src/tests/)
#add_subdirectory(src/ycvm/)

add_subdirectory(src/tools/pkgman/)
