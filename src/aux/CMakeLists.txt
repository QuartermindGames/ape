add_library(ape-aux STATIC
        private/pack/pack_pkg.c

        private/aux.c
        private/aux_collision.c
        private/aux_math.c
        private/aux_profiler.c
        private/aux_project.c
        private/aux_texture_packer.c
)

target_include_directories(ape-aux PRIVATE private ../3rdparty/miniz/)

set_target_properties(ape-aux PROPERTIES
        FOLDER "Engine"
        PREFIX ""
)
target_compile_definitions(ape-aux PRIVATE COMMON_DLL=1)
target_link_libraries(ape-aux acm qm-os plcore)

####################################################################
# Tests
####################################################################

find_package(PkgConfig)
if (PKGCONFIG_FOUND)
        pkg_check_modules(GLIB glib-2.0)
        pkg_check_modules(CAIRO cairo)
endif()

add_executable(ape-aux_test test/aux_test.c)
target_link_libraries(ape-aux_test
        acm
        qm-os
        plcore
        ape-aux

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
