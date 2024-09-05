if (WIN32 AND MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
endif ()
if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "forge.rc")
endif ()

add_library(Forge STATIC
        forge.cpp
        forge_about_dialog.cpp
        forge_project_dialog.cpp
        forge_console_frame.cpp
        forge_viewport.cpp
        forge_window_main.cpp

        forge_editor_tab.cpp forge_editor_tab.h
        editors/editor_material.cpp editors/editor_material.h

        forge_model_editor.cpp forge_model_editor.h
        forge_model_viewport.cpp forge_model_viewport.h

        editors/WorldEditor.cpp editors/WorldEditor.h
)

add_dependencies(Forge
        plcore
        plgraphics
        pl_gl_driver
        ApeCore
        FOX-1.6)

set_target_properties(Forge PROPERTIES
        FOLDER "Shells")

if (NOT UNIX)
    target_include_directories(Forge PRIVATE "glew/include/")
    target_link_directories(Forge PRIVATE "glew/lib/Release/x64/")
    target_link_libraries(Forge OpenGL32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(Forge PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()
else ()
    #target_link_libraries(forge GLEW GL GLU)
endif ()

target_link_libraries(Forge FOX-1.6)
target_include_directories(Forge
        PRIVATE ../kernel/plcore/include
        PRIVATE ../kernel/plgraphics/include
        PRIVATE ../common/public
        PRIVATE ../acm/public
        PRIVATE ../core/public
)
