if (WIN32 AND MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
endif ()

project(forge
        VERSION 0.1.0
        LANGUAGES C CXX)

if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "forge.rc")
endif ()

add_executable(forge WIN32
        editor.cpp
        editor_dialog_about.cpp
        editor_dialog_project.cpp
        editor_frame_console.cpp
        editor_frame_viewport.cpp
        editor_window_main.cpp
        editor_window_material.cpp
        editor_window_model.cpp)

add_dependencies(forge
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        
        ApeCore
        
        FOX-1.6)

set_target_properties(forge PROPERTIES
        FOLDER "Shells")

if (NOT UNIX)
    target_include_directories(forge PRIVATE "glew/include/")
    target_link_directories(forge PRIVATE "glew/lib/Release/x64/")
    target_link_libraries(forge OpenGL32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(forge PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()
else ()
    target_link_libraries(forge GLEW GL GLU)
endif ()

target_link_libraries(forge
        ApeCore
        FOX-1.6)