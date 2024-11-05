set(CMAKE_C_STANDARD 11)

add_library(acm STATIC
        private/acm.c
        private/acm_lexer.c
        private/acm_parser.c
        private/acm_preprocessor.c
)
target_include_directories(acm PUBLIC public)
target_link_libraries(acm ApeCommon plcore plgraphics)
