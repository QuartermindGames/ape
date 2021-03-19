# Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(engine)

set(CMAKE_C_STANDARD 11)

file(GLOB ENGINE_SOURCE_FILES
        *.c *.h
        )

add_library(engine SHARED ${ENGINE_SOURCE_FILES}
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

        ../3rdparty/miniz/miniz.c

        scheduler.h
        scheduler.c
        GameScript.c
        GameScript.h
        model_gsmdl.c
        model_md2.c
        MemoryManager.h MemoryManager.c

        integrations/DiscordIntegration.c
        )

target_include_directories(engine PRIVATE ./ ../3rdparty/miniz/)
target_link_directories(engine PRIVATE ../3rdparty/tcc-0.9.27/)

set_target_properties(engine PROPERTIES PREFIX "")

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(engine mingw32)
endif ()

target_link_libraries(engine common platform libtcc1.a SDL2)

file(COPY public/GameInterface.h DESTINATION ${CMAKE_HOME_DIRECTORY}/release/scripts/)
