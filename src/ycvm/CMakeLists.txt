# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

cmake_minimum_required(VERSION 3.5.1)

project(yc)

set(CMAKE_C_STANDARD 11)

add_executable(yc
        yc.c yc.h
        yvm.c yvm.h ycvm.h)

target_link_libraries(yc plcore)
