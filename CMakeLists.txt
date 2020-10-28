# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( Yin )

include( DetermineTargetArchitecture.cmake )

message( "Configuring Yin..." )

# Set all of our output directories.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")

include_directories( src/ )

add_subdirectory( src/3rdparty/platform/platform/ )
add_subdirectory( src/launcher_sdl2/ )
add_subdirectory( src/engine/ )
