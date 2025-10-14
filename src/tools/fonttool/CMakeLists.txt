if(NOT WIN32)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GLIB REQUIRED glib-2.0)
        pkg_check_modules(PANGO REQUIRED pango)
        pkg_check_modules(PANGOCAIRO REQUIRED pangocairo)
        pkg_check_modules(CAIRO REQUIRED cairo)

        add_executable(fonttool WIN32 fonttool.cpp)
        set_target_properties(fonttool PROPERTIES FOLDER "Tools")

        target_include_directories(fonttool PRIVATE
                ${PANGO_INCLUDE_DIRS}
                ${PANGOCAIRO_INCLUDE_DIRS}
                ${CAIRO_INCLUDE_DIRS})

        target_link_libraries(fonttool
                ${GLIB_LIBRARIES}
                ${PANGO_LIBRARIES}
                ${PANGOCAIRO_LIBRARIES}
                ${CAIRO_LIBRARIES}
                FOX-1.6
                plcore
                ape-common
        )
endif()