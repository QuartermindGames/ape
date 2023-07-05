if(WIN32 AND MSVC)
    set(APP_ICON_RESOURCE_WINDOWS "launcher.rc")
endif()

add_executable(ApeSDL2Shell WIN32
    launcher.c
    ${APP_ICON_RESOURCE_WINDOWS}
)
add_dependencies(ApeSDL2Shell
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        ApeCore
        )

set_target_properties(ApeSDL2Shell PROPERTIES
    FOLDER "Shells"
)

# Setup link libraries

if (MINGW)
    target_link_libraries(ApeSDL2Shell
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
endif ()

target_link_libraries(ApeSDL2Shell ApeCore)
