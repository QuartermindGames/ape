find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(fonttool WIN32
        fonttool.c
)

target_link_libraries(fonttool
        PkgConfig::GTK4
        plcore
        ApeCommon
)