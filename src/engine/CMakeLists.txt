# Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( engine )

set( CMAKE_C_STANDARD 11 )

file( GLOB ENGINE_SOURCE_FILES
        *.c *.h
        )

add_library( engine SHARED ${ENGINE_SOURCE_FILES}
        actors/actor_player.c
        actors/actor_trigger_volume.c
        renderer/camera.c
        renderer/font.c
        renderer/material.c renderer/material.h
        renderer/renderer.c
        renderer/scenegraph.c renderer/scenegraph.h
        audio.c audio.h

        ../3rdparty/miniz/miniz.c
        )

determine_target_architecture( engine )

target_include_directories( engine PRIVATE ./ ../3rdparty/miniz/ )

# Setup link libraries
if ( NOT UNIX )
    message( "Not Unix, assuming Win32 - " ${ARCH} )
    target_include_directories( engine PRIVATE src/3rdparty/sdl2/include/ )
    #if ( ${ARCH} MATCHES x86_64 )
    target_link_directories( engine PRIVATE src/3rdparty/sdl2/lib/x64/ )
    #else()
    #    target_link_directories( Yin PRIVATE src/3rdparty/sdl2/lib/x86/ )
    #endif()
    target_link_libraries( engine mingw32 )
endif()

target_link_libraries( engine platform SDL2 )
