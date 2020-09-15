# Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( engine )

set( CMAKE_C_STANDARD 11 )

file( GLOB ENGINE_SOURCE_FILES
        *.c *.h
        renderer/camera.c
        renderer/font.c
        renderer/material.c
        renderer/material.h
        renderer/renderer.c
        renderer/scenegraph.c
        renderer/scenegraph.h
        )

add_library( engine SHARED ${ENGINE_SOURCE_FILES} )

determine_target_architecture( engine )

target_include_directories( engine PRIVATE ./ )

# Setup link libraries
if ( NOT UNIX )
    message( "Not Unix, assuming Win32 - " ${ARCH} )
    target_link_libraries( engine mingw32 )
endif()

target_link_libraries( engine platform )
