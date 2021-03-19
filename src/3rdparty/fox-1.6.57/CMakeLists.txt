cmake_minimum_required(VERSION 3.5.1)

project(FOX-1.6)

file(GLOB FOX_SOURCE_FILES src/*.cpp src/*.h)

include_directories(include/)

add_library(FOX-1.6 STATIC ${FOX_SOURCE_FILES})

target_include_directories(FOX-1.6 PRIVATE src/)
target_compile_definitions(FOX-1.6 PRIVATE
        UNICODE
        HAVE_GL_H
        HAVE_GLU_H
        HAVE_STRTOLL
        HAVE_STRTOULL
        HAVE_VSSCANF)

set_target_properties(FOX-1.6 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
