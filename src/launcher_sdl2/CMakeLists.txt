# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(OSLauncherSDL2)

set(CMAKE_C_STANDARD 11)

add_executable(OSLauncherSDL2 launcher.c)

# Setup link libraries
if (NOT UNIX)
    target_include_directories(OSLauncherSDL2 PRIVATE src/3rdparty/sdl2/include/)
    if (${YIN_SYSTEM_ARCH} MATCHES x86_64)
        target_link_directories(OSLauncherSDL2 PRIVATE src/3rdparty/sdl2/lib/x64/)
    else ()
        target_link_directories(OSLauncherSDL2 PRIVATE src/3rdparty/sdl2/lib/x86/)
    endif ()
    target_link_libraries(OSLauncherSDL2 mingw32)
endif ()
target_link_libraries(OSLauncherSDL2 platform SDL2main SDL2)
