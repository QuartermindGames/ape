# Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

cmake_minimum_required(VERSION 3.5.1)

project(game)

set(CMAKE_C_STANDARD 11)

add_library(game SHARED
        game.c game.h)

set_target_properties(game PROPERTIES PREFIX "")

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(game mingw32)
endif ()

target_link_libraries(game plcore common)
