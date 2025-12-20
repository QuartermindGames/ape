cmake_minimum_required(VERSION 3.23)

set(CMAKE_VERBOSE_MAKEFILE ON)

project(Ape
        HOMEPAGE_URL "https://snortysoft.net/"
        DESCRIPTION "3D game engine written in C."
        LANGUAGES C CXX
)

# Attempt to figure out which compiler we're using...
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(APE_COMPILER "clang")
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(APE_COMPILER "gnu")
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(APE_COMPILER "msvc")
endif()
message("Compiler detected as ${APE_COMPILER}")

option(APE_BUILD_TOOLS "Build tools" ON)
if (NOT APE_BUILD_TOOLS)
    add_compile_definitions(-DAPE_NO_EDITOR)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set all of our output directories.

include_directories(src/)
include_directories(src/libcommon/include/)

include(DetermineTargetArchitecture.cmake)
determine_target_architecture(APE_SYSTEM_ARCH)

if (UNIX AND NOT APPLE)
    message("Assuming Unix - " ${APE_SYSTEM_ARCH})
    set(APE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/unix-${APE_SYSTEM_ARCH}/")
elseif (WIN32)
    message("Assuming Win32 - " ${APE_SYSTEM_ARCH})
    set(APE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/win32-${APE_SYSTEM_ARCH}/")
elseif (APPLE)
    message("Assuming Apple - " ${APE_SYSTEM_ARCH})
    set(APE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/apple-${APE_SYSTEM_ARCH}/")
else ()
    message("Unknown platform - " ${APE_SYSTEM_ARCH})
    set(APE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release/runtime/${APE_SYSTEM_ARCH}/")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APE_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${APE_OUTPUT_DIR}")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${APE_OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${APE_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${APE_OUTPUT_DIR})
endforeach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})

####################################################################
# 3rd party libraries

add_compile_definitions(AL_LIBTYPE_STATIC)
set(LIBTYPE STATIC)

set(ALSOFT_EXAMPLES OFF CACHE BOOL "")
set(ALSOFT_UTILS OFF CACHE BOOL "")
add_subdirectory(src/3rdparty/openal-soft)
if (WIN32)
    set_target_properties(alsoft.build_version PROPERTIES FOLDER "3rdParty/OpenAL")
    set_target_properties(alsoft.common PROPERTIES FOLDER "3rdParty/OpenAL")
    set_target_properties(alsoft.excommon PROPERTIES FOLDER "3rdParty/OpenAL")
    set_target_properties(alsoft.fmt PROPERTIES FOLDER "3rdParty/OpenAL")
endif ()
set_target_properties(OpenAL PROPERTIES FOLDER "3rdParty/OpenAL")

add_subdirectory(src/3rdparty/fox)
set_target_properties(FOX-1.6 PROPERTIES FOLDER "3rdParty")

add_subdirectory(src/3rdparty/lua)
set_target_properties(lua PROPERTIES FOLDER "3rdParty")

####################################################################
# main projects

if (WIN32)
    if (${APE_SYSTEM_ARCH} MATCHES x86_64)
        if (MINGW)
            link_directories("src/3rdparty/sdl3/lib/x64_mingw/")
        else ()
            link_directories("src/3rdparty/sdl3/lib/x64/")
        endif ()
    else ()
        if (MINGW)
            link_directories("src/3rdparty/sdl3/lib/x86_mingw/")
        else ()
            link_directories("src/3rdparty/sdl3/lib/x86/")
        endif ()
    endif ()
    include_directories("src/3rdparty/sdl3/include/")
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

if (APE_COMPILER STREQUAL "msvc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3 -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3 -D_CRT_SECURE_NO_WARNINGS")
elseif (APE_COMPILER STREQUAL "gnu")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-variable -Wno-unused-function -Werror=return-type -Werror=implicit-function-declaration")
elseif (APE_COMPILER STREQUAL "clang")
    add_compile_options("-Wno-unused-function")
    if(WIN32)
        add_compile_definitions("_CRT_SECURE_NO_WARNINGS")
    endif()
endif ()

# QMFW libraries
add_subdirectory(src/kernel/src/qmos)
set_target_properties(qm-os PROPERTIES FOLDER "Kernel")
add_subdirectory(src/kernel/src/qmmath)
set_target_properties(qm-math PROPERTIES FOLDER "Kernel")

add_subdirectory(src/kernel/plcore/)
set_target_properties(plcore PROPERTIES FOLDER "Kernel")
add_subdirectory(src/kernel/plgraphics/)
set_target_properties(plgraphics PROPERTIES FOLDER "Kernel")
add_subdirectory(src/kernel/plugins/driver_opengl/)
set_target_properties(pl_gl_driver PROPERTIES FOLDER "Kernel")

set(HEI_LIBRARIES qm-os qm-math plcore plgraphics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 23)

if (MSVC)
    if (${APE_SYSTEM_ARCH} MATCHES x86_64)
        link_directories(src/3rdparty/sdl2/lib/x64)
    else ()
        link_directories(src/3rdparty/sdl2/lib/x86)
    endif ()
elseif (MINGW)
    link_directories(src/3rdparty/sdl2/lib/x64_mingw)
endif ()

####################################################################

add_subdirectory(src/common)
add_subdirectory(src/core)

add_subdirectory(src/acm)
set_target_properties(acm PROPERTIES FOLDER "Kernel")

if (APE_BUILD_TOOLS)
    add_subdirectory(src/forge)
    set_target_properties(Forge PROPERTIES FOLDER "Tools")

    add_subdirectory(src/tools/cook)
    set_target_properties(cook PROPERTIES FOLDER "Tools")

    add_subdirectory(src/tools/fonttool)

    add_subdirectory(src/tools/matgen)
    set_target_properties(matgen PROPERTIES FOLDER "Tools")
endif ()
add_subdirectory(src/game) # includes shells
