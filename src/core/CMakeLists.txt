# Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

cmake_minimum_required(VERSION 3.5.1)

project(OSCore)

set(CMAKE_C_STANDARD 11)

add_library(OSCore SHARED
        lightmapper.c
        wld.c
        include/core/core.h
        include/core/wld.h
        )

target_include_directories(OSCore PRIVATE ./ ../3rdparty/miniz/)

# Setup link libraries
if (NOT UNIX)
    set_target_properties(OSCore PROPERTIES PREFIX "")
    target_link_libraries(OSCore mingw32)
endif ()

target_link_libraries(OSCore platform)
