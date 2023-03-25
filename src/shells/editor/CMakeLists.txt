if (WIN32 AND MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
endif ()

project(yin-editor
        VERSION 0.1.0
        LANGUAGES C CXX)

add_executable(yin-editor WIN32
        editor.cpp
        editor_frame_console.cpp
        editor_frame_viewport.cpp
        editor_window_main.cpp
        editor_window_model.cpp)

add_dependencies(yin-editor
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        yin-core
        FOX-1.6)

set_target_properties(yin-editor PROPERTIES
        FOLDER "Shells")

if (NOT UNIX)
    target_include_directories(yin-editor PRIVATE "glew/include/")
    target_link_directories(yin-editor PRIVATE "glew/lib/Release/x64/")
    target_link_libraries(yin-editor OpenGL32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(yin-editor PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()
else ()
    target_link_libraries(yin-editor GLEW GL GLU)
endif ()

target_link_libraries(yin-editor
        yin-core
        FOX-1.6)