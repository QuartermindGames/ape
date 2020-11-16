# Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(OSEngine)

set(CMAKE_C_STANDARD 11)

file(GLOB ENGINE_SOURCE_FILES
        *.c *.h
        )

add_library(OSEngine SHARED ${ENGINE_SOURCE_FILES}
        actors/actor_light.c
        actors/actor_player.c
        actors/actor_trigger_volume.c

        renderer/camera.c
        renderer/flare.c
        renderer/flare.h
        renderer/font.c
        renderer/material.c
        renderer/material.h
        renderer/renderer.c
        renderer/scenegraph.c
        renderer/scenegraph.h
        audio.c
        audio.h
        script.c
        script.h

        ../3rdparty/miniz/miniz.c)

target_include_directories(OSEngine PRIVATE ./ ../3rdparty/miniz/)

# Setup link libraries
if (NOT UNIX)
    message("Not Unix, assuming Win32 - " ${YIN_SYSTEM_ARCH})
    set_target_properties(OSEngine PROPERTIES PREFIX "")
    target_include_directories(OSEngine PRIVATE src/3rdparty/sdl2/include/)
    if (${YIN_SYSTEM_ARCH} MATCHES "x86_64")
        set_target_properties(OSEngine PROPERTIES OUTPUT_NAME OSEngine_x64)
        target_link_directories(OSEngine PRIVATE src/3rdparty/sdl2/lib/x64/)
    else ()
        set_target_properties(OSEngine PROPERTIES OUTPUT_NAME OSEngine_x86)
        target_link_directories(OSEngine PRIVATE src/3rdparty/sdl2/lib/x86/)
    endif ()
    target_link_libraries(OSEngine mingw32)
endif ()

target_link_libraries(OSEngine OSCore platform SDL2)
