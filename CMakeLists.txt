# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( Yin )

include( DetermineTargetArchitecture.cmake )

message( "Configuring Yin..." )

set( CMAKE_C_STANDARD 11 )

# Set all of our output directories.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")

add_subdirectory( src/3rdparty/platform/platform )
add_subdirectory( tools/pkgman/ )

file( GLOB YIN_SOURCE_FILES
        src/*.c
        src/*.h )

add_executable( Yin ${YIN_SOURCE_FILES} )

determine_target_architecture( Yin )

if ( NOT UNIX )
    message( "Not Unix, assuming Win32 - " ${ARCH} )
    target_include_directories( Yin PRIVATE src/3rdparty/sdl2/include/ )
    #if ( ${ARCH} MATCHES x86_64 )
        target_link_directories( Yin PRIVATE src/3rdparty/sdl2/lib/x64/ )
    #else()
    #    target_link_directories( Yin PRIVATE src/3rdparty/sdl2/lib/x86/ )
    #endif()
endif()

target_link_libraries( Yin platform SDL2 SDL2main )
