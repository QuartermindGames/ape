set(CMAKE_C_STANDARD 11)

add_library(ApeNode STATIC
        private/node.c
        private/node_ds_common.c
        private/node_parser.c
        private/node_preprocessor.c
)
target_include_directories(ApeNode PUBLIC ../node/public)
target_link_libraries(ApeNode ApeCommon plcore plgraphics)
