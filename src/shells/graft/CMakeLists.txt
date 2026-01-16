find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

add_executable(graft WIN32
        graft.c
)

target_link_libraries(graft
        ape-core

        ${GTK4_LIBRARIES}
)

target_include_directories(graft PRIVATE
        ${GTK4_INCLUDE_DIRS}
)

set_target_properties(graft PROPERTIES FOLDER "Tools")
