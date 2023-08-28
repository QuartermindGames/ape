cmake_minimum_required(VERSION 3.5.1)

project(dickens
        LANGUAGES C
        DESCRIPTION "Simple scripting language."
)

set(CMAKE_C_STANDARD 11)

add_library(dickens SHARED
        private/dickens_assembler.c
        private/dickens_builtins.c
        private/dickens_parser.c
        private/dickens_compiler.c
        private/dickens_lexer.c
)

set_target_properties(dickens PROPERTIES FOLDER "Scripting/Dickens")

target_include_directories(dickens
        PRIVATE private/
        PUBLIC public/
)
target_link_libraries(dickens plcore ape-common ape-node)
