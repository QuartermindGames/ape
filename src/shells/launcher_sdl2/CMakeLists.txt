if(WIN32 AND MSVC)
    set(APP_ICON_RESOURCE_WINDOWS "launcher.rc")
endif()

add_executable(yin-shell_sdl2 WIN32
    launcher.c
    ${APP_ICON_RESOURCE_WINDOWS}
)
add_dependencies(yin-shell_sdl2
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        yin-core
        )

set_target_properties(yin-shell_sdl2 PROPERTIES
    FOLDER "Shells"
)

# Setup link libraries

if (MINGW)
    target_link_libraries(yin-shell_sdl2
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
endif ()

target_link_libraries(yin-shell_sdl2
        yin-core)
