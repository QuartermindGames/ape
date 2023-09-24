cmake_minimum_required(VERSION 3.23)

project(Dickens
        LANGUAGES C
        DESCRIPTION "Simple scripting language."
)

set(CMAKE_C_STANDARD 11)

add_subdirectory(lib)
add_subdirectory(dcmd)
