cmake_minimum_required(VERSION 3.5.1)

project(FOX-1.6)

include(CheckIncludeFile)
include(CheckFunctionExists)

file(GLOB FOX_SOURCE_FILES src/*.cpp src/*.h)

include_directories(include/)

add_library(FOX-1.6 STATIC ${FOX_SOURCE_FILES})

check_include_file(GL/gl.h HAVE_GL_H)
check_include_file(GL/glu.h HAVE_GLU_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(unistd.h HAVE_UNISTD_H)

check_function_exists(vsscanf HAVE_VSSCANF)
check_function_exists(strtoll HAVE_STRTOLL)
check_function_exists(strtoull HAVE_STRTOULL)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/FXBuildConfig.h)

target_include_directories(FOX-1.6 PRIVATE src/)
target_compile_definitions(FOX-1.6 PRIVATE UNICODE)
if(UNIX)
    target_link_libraries(FOX-1.6 X11 Xi Xxf86vm pthread)
endif(UNIX)

set_target_properties(FOX-1.6 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
