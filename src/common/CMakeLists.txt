add_library(ape-common STATIC
        private/common.c
        private/common_collision.c
        private/common_math.c
        private/common_profiler.c
        private/common_project.c
        private/pack/pack_pkg.c
)

target_include_directories(ape-common PRIVATE private ../3rdparty/miniz/)
target_include_directories(ape-common PUBLIC public/)

set_target_properties(ape-common PROPERTIES
        FOLDER "Engine"
        PREFIX ""
)
target_compile_definitions(ape-common PRIVATE COMMON_DLL=1)
target_link_libraries(ape-common
        acm
        qm-os 
        plcore
)
