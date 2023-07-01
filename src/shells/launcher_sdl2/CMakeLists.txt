if(WIN32 AND MSVC)
    set(APP_ICON_RESOURCE_WINDOWS "launcher.rc")
endif()

add_executable(ApeShellSDL2 WIN32
    launcher.c
    ${APP_ICON_RESOURCE_WINDOWS}
)
add_dependencies(ApeShellSDL2
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        ApeCore
        )

set_target_properties(ApeShellSDL2 PROPERTIES
    FOLDER "Shells"
)

# Setup link libraries

if (MINGW)
    target_link_libraries(ApeShellSDL2
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
endif ()

target_link_libraries(ApeShellSDL2 ApeCore)
