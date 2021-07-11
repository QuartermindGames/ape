# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(launcher_sdl2)

set(CMAKE_C_STANDARD 11)

add_executable(launcher_sdl2 launcher.c)

# Setup link libraries
#[[
if (NOT UNIX AND NOT MSVC)
   target_link_libraries(launcher_sdl2 mingw32)
endif ()
]]

target_link_libraries(launcher_sdl2 plcore common SDL2 SDL2main)
