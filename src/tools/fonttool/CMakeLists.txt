find_package(GTK2 REQUIRED gtk)
if(GTK2_FOUND)
    include_directories(${GTK2_INCLUDE_DIRS})

    add_executable(fonttool WIN32
            fonttool.c)

    target_link_libraries(fonttool ${GTK2_LIBRARIES}
            plcore
            ApeCommon)
endif()
