add_library(yin-common STATIC
        private/common.c
        private/pack/pack_pkg.c
        private/pack/pack_vpp.c)

target_include_directories(yin-common PRIVATE ../3rdparty/miniz/)
target_include_directories(yin-common PUBLIC public/)

set_target_properties(yin-common PROPERTIES
    FOLDER "Engine"
    PREFIX ""
)
target_compile_definitions(yin-common PRIVATE COMMON_DLL=1)
target_link_libraries(yin-common yin-node plcore)
