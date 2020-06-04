# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( Yin )

include( DetermineTargetArchitecture.cmake )

message( "Configuring Yin..." )

set( CMAKE_C_STANDARD 11 )

# Set all of our output directories.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")

include_directories( src/ )

add_subdirectory( src/3rdparty/platform/platform/ )
add_subdirectory( src/launcher_sdl2/ )
add_subdirectory( tools/pkgman/ )

file( GLOB YIN_SOURCE_FILES
        src/engine/*.c
        src/engine/*.h )

add_executable( Yin WIN32 ${YIN_SOURCE_FILES} )

determine_target_architecture( Yin )

if ( NOT UNIX )
    message( "Not Unix, assuming Win32 - " ${ARCH} )
    target_link_libraries( Yin mingw32 )
endif()

target_link_libraries( Yin platform )
