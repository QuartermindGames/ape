# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( launcher )

set( CMAKE_C_STANDARD 11 )

add_executable( launcher launcher.c )

determine_target_architecture( launcher )

# Setup link libraries
if ( NOT UNIX )
    message( "Not Unix, assuming Win32 - " ${ARCH} )
    target_include_directories( launcher PRIVATE src/3rdparty/sdl2/include/ )
    #if ( ${ARCH} MATCHES x86_64 )
    target_link_directories( launcher PRIVATE src/3rdparty/sdl2/lib/x64/ )
    #else()
    #    target_link_directories( Yin PRIVATE src/3rdparty/sdl2/lib/x86/ )
    #endif()
    target_link_libraries( launcher mingw32 )
endif()
target_link_libraries( launcher SDL2main SDL2 )
