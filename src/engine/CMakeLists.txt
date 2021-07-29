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
        actors/sg_actors.c

        client/client.c
        client/sgui.c
        client/audio/audio.c
        client/audio/micromod.c
        client/audio/sdlplayer.c
        client/renderer/camera.c
        client/renderer/flare.c
        client/renderer/font.c
        client/renderer/material.c
        client/renderer/particle.c
        client/renderer/renderer.c
        client/renderer/scenegraph.c
        client/renderer/shaders.c
        client/renderer/sprite.c
        client/renderer/texture.c

        script.c
        scheduler.c

        ../3rdparty/miniz/miniz.c
        
        memory_manager.c
        net_server.c

        integrations/DiscordIntegration.c)

target_include_directories(engine PRIVATE
        ./ ../3rdparty/miniz/)

set_target_properties(engine PROPERTIES PREFIX "")

# Setup link libraries
if (NOT UNIX AND NOT MSVC)
    target_link_libraries(engine mingw32)
endif ()

target_link_libraries(engine common plcore plgraphics plmodel SDL2)
