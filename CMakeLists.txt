# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.23)

set(CMAKE_VERBOSE_MAKEFILE ON)

project(Ape
        LANGUAGES C CXX
        )

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

if (WIN32)
    if (${APE_SYSTEM_ARCH} MATCHES x86_64)
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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(MSVC)
    if (${APE_SYSTEM_ARCH} MATCHES x86_64)
        link_directories(src/3rdparty/sdl2/lib/x64)
    else()
        link_directories(src/3rdparty/sdl2/lib/x86)
    endif()
elseif(MINGW)
    link_directories(src/3rdparty/sdl2/lib/x64_mingw)
endif()

add_subdirectory(src/dickens)

add_subdirectory(src/common/)
add_subdirectory(src/core/)
add_subdirectory(src/node/)
add_subdirectory(src/game/)

add_subdirectory(src/shells/launcher_sdl2/)

# tools
add_subdirectory(src/tools/cook)
add_subdirectory(src/tools/fonttool)
add_subdirectory(src/tools/matgen)
add_subdirectory(src/tools/m2w)

add_subdirectory(src/tests/)
