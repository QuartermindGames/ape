add_library(dlib SHARED
        private/dickens_assembler.c
        private/dickens_builtins.c
        private/dickens_parser.c
        private/dickens_compiler.c
        private/dickens_lexer.c
)

set_target_properties(dlib PROPERTIES FOLDER "Scripting/Dickens")

target_include_directories(dlib
        PRIVATE private/
        PUBLIC public/
)
target_link_libraries(dlib plcore)
