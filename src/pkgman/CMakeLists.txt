# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( pkgman )

set( CMAKE_C_STANDARD 11 )

add_executable( pkgman
        pkgman.c
        pack_image.c
        ../3rdparty/miniz/miniz.c
        parser.c parser.h)

target_include_directories( pkgman PRIVATE ../3rdparty/miniz/ )
target_link_libraries( pkgman platform )
