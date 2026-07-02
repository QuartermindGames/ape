find_package(Qt6 REQUIRED COMPONENTS Core)

if (WIN32 AND MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
endif ()
if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "forge.rc")
endif ()

add_library(ape-editor STATIC
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
set_target_properties(ape-editor PROPERTIES FOLDER "Engine")

add_dependencies(ape-editor
        plcore
        plgraphics
        pl_gl_driver
        ape-core
)

if (NOT UNIX)
    target_include_directories(ape-editor PRIVATE "glew/include/")
    target_link_directories(ape-editor PRIVATE "glew/lib/Release/x64/")
    target_link_libraries(ape-editor OpenGL32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(ape-editor PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()
endif ()

target_link_libraries(ape-editor
        qm-os
        qm-math

        FOX-1.6
        acm

        Qt6::Core
)
target_include_directories(ape-editor PRIVATE
        ../kernel/plcore/include
        ../kernel/plgraphics/include
        ../common/public
        ../acm/public
        ../core/public
)
