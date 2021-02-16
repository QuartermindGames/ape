# Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

cmake_minimum_required(VERSION 3.5.1)

project(common)

set(CMAKE_C_STANDARD 11)

add_library(common SHARED
        lightmapper.c
        wld.c
        include/common/common.h
        include/common/wld.h
        Node.c
        include/common/Node.h
        NodePrivate.h
        NodePreProcessor.c Common.c)

target_include_directories(common PRIVATE ./ ../3rdparty/miniz/)

set_target_properties(common PROPERTIES PREFIX "")
target_compile_definitions(common PRIVATE COMMON_DLL=1)

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(common mingw32)
endif ()

target_link_libraries(common platform)
