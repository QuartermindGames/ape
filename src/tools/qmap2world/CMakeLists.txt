# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(qmap2world)

set(CMAKE_C_STANDARD 11)

add_executable(qmap2world
        qmap2world.c
        )

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(qmap2world mingw32)
endif ()
target_link_libraries(qmap2world platform)
