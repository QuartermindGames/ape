# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required( VERSION 3.5.1 )

project( pkgman )

set( CMAKE_C_STANDARD 11 )

add_executable( pkgman main.c pack_image.c )

target_link_libraries( pkgman platform )


