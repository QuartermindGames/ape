find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(ApeMatEd
        mated.c
)

set_target_properties(ApeMatEd PROPERTIES
        FOLDER "Shells"
)

target_include_directories(ApeMatEd PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_libraries(ApeMatEd
        ${GTK4_LIBRARIES}
        ApeCore
)
