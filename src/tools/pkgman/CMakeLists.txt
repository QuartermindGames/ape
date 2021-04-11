# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(pkgman)

set(CMAKE_C_STANDARD 11)

add_executable(pkgman
        pack_image.c
        pkgman.c
        pkgman.h
        parser.c
        parser.h
        txc_compress_dxtn.c
        txc_dxtn.h
        ../../3rdparty/miniz/miniz.c)

file(COPY pkgman_legal.txt DESTINATION ${CMAKE_HOME_DIRECTORY}/release/docs/)

target_include_directories(pkgman PRIVATE ../../3rdparty/miniz)
target_link_libraries(pkgman platform)
