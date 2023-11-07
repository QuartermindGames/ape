add_library(sscript STATIC
        script.c
        script_lexer.c
)
target_include_directories(sscript PUBLIC public)
target_link_libraries(sscript plcore)
