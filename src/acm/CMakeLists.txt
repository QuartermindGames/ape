set(CMAKE_C_STANDARD 11)

file(GLOB ACM_SOURCE_FILES
        private/acm.c
        private/acm_lexer.c
        private/acm_parser.c
        private/acm_preprocessor.c
)

add_library(acm STATIC ${ACM_SOURCE_FILES})
target_include_directories(acm PUBLIC public)
target_link_libraries(acm ApeCommon plcore plgraphics)

add_executable(acm_test ${ACM_SOURCE_FILES})
target_compile_definitions(acm_test PRIVATE ACM_TEST)
target_link_libraries(acm_test acm)
