# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( Yin )

set( CMAKE_C_STANDARD 11 )

# Set all of our output directories.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")

add_subdirectory( src/3rdparty/platform/platform )
add_subdirectory( tools/pkgman/ )

file( GLOB YIN_SOURCE_FILES
        src/*.c
        src/*.h )

add_executable( Yin WIN32 ${YIN_SOURCE_FILES} )

if ( !UNIX )
    target_include_directories( Yin PRIVATE src/3rdparty/sdl2/include/ )
endif()

target_link_libraries( Yin platform SDL2 )
