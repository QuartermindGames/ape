if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "launcher.rc")
endif ()

add_executable(ape-shell-sdl2 WIN32
        launcher.c
        ${APP_ICON_RESOURCE_WINDOWS}
)
add_dependencies(ape-shell-sdl2
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        ApeCore
)

set_target_properties(ape-shell-sdl2 PROPERTIES
        FOLDER "Shells"
)

# Setup link libraries

if (MINGW)
    target_link_libraries(ape-shell-sdl2
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
endif ()

target_link_libraries(ape-shell-sdl2 ApeCore)
set_target_properties(ape-shell-sdl2 PROPERTIES OUTPUT_NAME "ape")
