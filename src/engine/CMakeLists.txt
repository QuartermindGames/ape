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
        actors/TriggerVolume.c

        renderer/camera.c
        renderer/flare.c
        renderer/font.c
        renderer/material.c
        renderer/renderer.c
        renderer/scenegraph.c
        renderer/shaders.c

        audio.c
        audio.h
        script.c
        script.h

        ../3rdparty/miniz/miniz.c

        scheduler.c
        model_gsmdl.c
        model_md2.c
        memory_manager.c

        integrations/DiscordIntegration.c
        )

target_include_directories(engine PRIVATE
        ./ ../3rdparty/miniz/)

set_target_properties(engine PROPERTIES PREFIX "")

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(engine mingw32)
endif ()

target_link_libraries(engine common plcore plgraphics plmodel SDL2)
