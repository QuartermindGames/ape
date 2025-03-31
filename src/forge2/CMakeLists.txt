find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(ADWAITA REQUIRED libadwaita-1)

add_executable(ape-forge2 WIN32
        forge.c
)

set_target_properties(ape-forge2 PROPERTIES FOLDER "Shells")

target_include_directories(ape-forge2 PRIVATE ${GTK4_STATIC_INCLUDE_DIRS} ${ADWAITA_STATIC_INCLUDE_DIRS})
target_link_libraries(ape-forge2 ${GTK4_STATIC_LIBRARIES} ${ADWAITA_STATIC_LIBRARIES})
