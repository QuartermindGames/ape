add_library(ApeCommon STATIC
        private/common.c
        private/common_profiler.c
        private/common_project.c
        private/pack/pack_pkg.c
)

target_include_directories(ApeCommon PRIVATE ../3rdparty/miniz/)
target_include_directories(ApeCommon PUBLIC public/)

set_target_properties(ApeCommon PROPERTIES
        FOLDER "Engine"
        PREFIX ""
)
target_compile_definitions(ApeCommon PRIVATE COMMON_DLL=1)
target_link_libraries(ApeCommon ApeNode plcore)
