add_library(ape-common STATIC
        private/aux_texture_packer.c
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

####################################################################
# Tests
####################################################################

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB glib-2.0)
pkg_check_modules(CAIRO cairo)

add_executable(ape-aux_test test/aux_test.c)
target_link_libraries(ape-aux_test
        acm
        qm-os
        plcore
        ape-common

        ${GLIB_LIBRARIES}
        ${CAIRO_LIBRARIES}
)
target_include_directories(ape-aux_test PRIVATE
        ${CAIRO_INCLUDE_DIRS}
)

set_target_properties(ape-aux_test PROPERTIES
        FOLDER "Tests"
        PREFIX ""
)

add_test(NAME ape-tests-aux COMMAND ape-aux_test)
