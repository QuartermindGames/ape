# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(bsp2geo)

set(CMAKE_C_STANDARD 11)

add_executable(bsp2geo
        bsp.c bsp.h
        bsp2geo.c bsp2geo.h)

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(bsp2geo mingw32)
endif ()
target_link_libraries(bsp2geo platform)
