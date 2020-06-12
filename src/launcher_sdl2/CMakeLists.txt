# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( launcher_sdl2 )

set( CMAKE_C_STANDARD 11 )

add_executable( launcher_sdl2 launcher.c )

determine_target_architecture( launcher_sdl2 )

# Setup link libraries
if ( NOT UNIX )
    message( "Not Unix, assuming Win32 - " ${ARCH} )
    target_include_directories( launcher_sdl2 PRIVATE src/3rdparty/sdl2/include/ )
    #if ( ${ARCH} MATCHES x86_64 )
    target_link_directories( launcher_sdl2 PRIVATE src/3rdparty/sdl2/lib/x64/ )
    #else()
    #    target_link_directories( Yin PRIVATE src/3rdparty/sdl2/lib/x86/ )
    #endif()
    target_link_libraries( launcher_sdl2 mingw32 )
endif()
target_link_libraries( launcher_sdl2 platform SDL2main SDL2 )
