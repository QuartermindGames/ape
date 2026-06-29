if (WIN32 AND MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
endif ()
if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "forge.rc")
endif ()

add_library(Forge STATIC
        private/forge.cpp
        private/forge_about_dialog.cpp
        private/forge_console_frame.cpp
        private/forge_project_dialog.cpp

        private/forge_viewport.cpp
        private/forge_viewport_world.cpp

        private/forge_window_main.cpp

        private/forge_editor_tab.cpp
        private/forge_editor_tab.h

        private/forge_model_editor.cpp
        private/forge_model_editor.h
        private/forge_model_viewport.cpp
        private/forge_model_viewport.h

        private/forge_editor_material.cpp
        private/forge_editor_world.cpp

        private/forge_browser_materials.cpp
        private/forge_dialog_properties.cpp

        private/SurfaceInspector.cpp
        private/UVFrame.cpp
)

add_dependencies(Forge
        plcore
        plgraphics
        pl_gl_driver
        ape-core
        FOX-1.6)

if (NOT UNIX)
    target_include_directories(Forge PRIVATE "glew/include/")
    target_link_directories(Forge PRIVATE "glew/lib/Release/x64/")
    target_link_libraries(Forge OpenGL32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(Forge PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()
endif ()

target_link_libraries(Forge
        qm-os
        qm-math

        FOX-1.6
        acm
)
target_include_directories(Forge PRIVATE
        ../kernel/plcore/include
        ../kernel/plgraphics/include
        ../common/public
        ../acm/public
        ../core/public
)
