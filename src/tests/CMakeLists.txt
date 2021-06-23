# Copyright (C) 2021 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(tests)

set(CMAKE_C_STANDARD 11)

add_executable(tests tests.c)

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(tests mingw32)
endif ()
target_link_libraries(tests plcore common)
