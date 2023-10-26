find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

if (WIN32 AND MSVC)
    set(APP_ICON_RESOURCE_WINDOWS "forge.rc")
endif ()

add_executable(forge WIN32
        forge.c
        ${APP_ICON_RESOURCE_WINDOWS}
)

add_dependencies(forge
        plcore
        plgraphics
        pl_gl_driver
        ApeCore
)

set_target_properties(forge PROPERTIES FOLDER "Shells")
target_link_libraries(forge PkgConfig::GTK4 ApeCore)
