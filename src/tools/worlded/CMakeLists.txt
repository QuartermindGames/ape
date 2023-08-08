find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(ApeWorldEd worlded.c)

target_include_directories(ApeWorldEd PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_libraries(ApeWorldEd ${GTK4_LIBRARIES})
