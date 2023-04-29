# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.23)

project(Yin
        LANGUAGES C CXX
        )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

message("Configuring Yin...")

# Set all of our output directories.

include_directories(src/)
include_directories(src/libcommon/include/)

include(DetermineTargetArchitecture.cmake)
determine_target_architecture(YIN_SYSTEM_ARCH)

if (UNIX AND NOT APPLE)
    message("Assuming Unix - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/unix-${YIN_SYSTEM_ARCH}/")
elseif (WIN32)
    message("Assuming Win32 - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/win32-${YIN_SYSTEM_ARCH}/")
elseif (APPLE)
    message("Assuming Apple - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/apple-${YIN_SYSTEM_ARCH}/")
else ()
    message("Unknown platform - " ${YIN_SYSTEM_ARCH})
    set(YIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/${YIN_SYSTEM_ARCH}/")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${YIN_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${YIN_OUTPUT_DIR}")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${YIN_OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${YIN_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${YIN_OUTPUT_DIR})
endforeach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})

if (WIN32)
    if (${YIN_SYSTEM_ARCH} MATCHES x86_64)
        if (MINGW)
            link_directories(src/3rdparty/sdl2/lib/x64_mingw/)
        else ()
            link_directories(src/3rdparty/sdl2/lib/x64/)
        endif ()
    else ()
        if (MINGW)
            link_directories(src/3rdparty/sdl2/lib/x86_mingw/)
        else ()
            link_directories(src/3rdparty/sdl2/lib/x86/)
        endif ()
    endif ()
    include_directories(src/3rdparty/sdl2/include/)
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3 -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3 -D_CRT_SECURE_NO_WARNINGS")
else ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-variable -Wno-unused-function -Werror=implicit-function-declaration")
endif ()

# Get the current working branch
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the latest abbreviated commit hash of the working branch
execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the number of commits on the working branch
execute_process(
        COMMAND git rev-list HEAD --count
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
add_definitions(-DGIT_COMMIT_COUNT="${GIT_COMMIT_COUNT}")
add_definitions(-DGIT_BRANCH="${GIT_BRANCH}")

# main projects

add_subdirectory(src/kernel/plcore/)
set_target_properties(plcore PROPERTIES FOLDER "Kernel")
add_subdirectory(src/kernel/plgraphics/)
set_target_properties(plgraphics PROPERTIES FOLDER "Kernel")
add_subdirectory(src/kernel/plmodel/)
set_target_properties(plmodel PROPERTIES FOLDER "Kernel")
add_subdirectory(src/kernel/plugins/driver_opengl/)
set_target_properties(pl_gl_driver PROPERTIES FOLDER "Kernel")

function(build_openal)
    set(LIBTYPE "STATIC")
    add_definitions("-DAL_LIBTYPE_STATIC")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/src/3rdparty/openal-soft/lib/")
    set(ALSOFT_UTILS OFF CACHE BOOL "Build and install utility programs")
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "Build and install example programs")
    set(ALSOFT_TESTS OFF CACHE BOOL "Build and install test programs")
    add_subdirectory(src/3rdparty/openal-soft)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(OpenAL PRIVATE "-w")
    endif()
    set_target_properties(OpenAL PROPERTIES FOLDER "3rdParty/OpenAL")
    set_target_properties(common PROPERTIES FOLDER "3rdParty/OpenAL")
    set_target_properties(ex-common PROPERTIES FOLDER "3rdParty/OpenAL")
    set_target_properties(build_version PROPERTIES FOLDER "3rdParty/OpenAL")
endfunction()

build_openal()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(MSVC)
    if (${YIN_SYSTEM_ARCH} MATCHES x86_64)
        link_directories(src/3rdparty/sdl2/lib/x64)
    else()
        link_directories(src/3rdparty/sdl2/lib/x86)
    endif()
elseif(MINGW)
    link_directories(src/3rdparty/sdl2/lib/x64_mingw)
endif()

add_subdirectory(src/common/)

add_subdirectory(src/core/)
add_subdirectory(src/node/)
add_subdirectory(src/game/)
add_subdirectory(src/gui/)

add_subdirectory(src/dickens/lib/)
add_subdirectory(src/dickens/dcmd/)

add_subdirectory(src/3rdparty/fox)

add_subdirectory(src/shells/launcher_sdl2/)
add_subdirectory(src/shells/editor/)

# tools
add_subdirectory(src/tests/)
add_subdirectory(src/pkgman/)
