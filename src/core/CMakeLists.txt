# Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

cmake_minimum_required(VERSION 3.5.1)

project(OSCore)

set(CMAKE_C_STANDARD 11)

add_library(OSCore SHARED
        lightmapper.c
        include/core/core.h
        include/core/format_wld.h
        )

target_include_directories(OSCore PRIVATE ./ ../3rdparty/miniz/)

# Setup link libraries
if (NOT UNIX)
    message("Not Unix, assuming Win32 - " ${YIN_SYSTEM_ARCH})
    set_target_properties(OSCore PROPERTIES PREFIX "")
    if (${YIN_SYSTEM_ARCH} MATCHES "x86_64")
        set_target_properties(OSCore PROPERTIES OUTPUT_NAME OSCore_x64)
    else()
        set_target_properties(OSCore PROPERTIES OUTPUT_NAME OSCore_x86)
    endif()
    target_link_libraries(OSCore mingw32)
endif ()

target_link_libraries(OSCore platform)
