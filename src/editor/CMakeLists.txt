# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

project(editor)

if (UNIX)
    set(UNIX_ADDITIONAL)
    file(GLOB UNIX_ADDITIONAL findfirst.c spec.c)
endif ()

add_executable(editor
        brush.cpp

        BasePerspective.cpp
        CameraPerspective.cpp

        cmdlib.cpp
        csg.cpp
        drag.cpp
        eclass.cpp
        entity.cpp
        Layer.cpp
        lbmlib.cpp
        World.cpp
        mathlib.cpp
        parse.cpp
        qe3.cpp
        select.cpp
        textures.cpp
        Utilities.cpp
        vertsel.cpp
        Viewport.cpp
        win_cam.cpp
        win_dlg.cpp
        win_ent.cpp
        win_qe3.cpp
        win_xy.cpp
        xy.cpp
        z.cpp

        # Windows
        MainWindow.cpp

        # Mostly for dir search functions
        ${UNIX_ADDITIONAL}
        )

target_include_directories(editor PRIVATE ../3rdparty/fox-1.6.57/include/)

if (NOT UNIX)
    target_include_directories(editor PRIVATE "glew/include/")
    target_link_directories(editor PRIVATE "glew/lib/Release/x64/")
    target_link_libraries(editor glew32s)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(platform PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()

    # link glew
    target_include_directories(editor PRIVATE "glew/include/")
    target_link_directories(editor PRIVATE "glew/lib/Release/x64/")
    target_link_libraries(editor glew32s)

else ()
    target_link_libraries(editor GLEW GL GLU)
endif ()

target_link_libraries(editor FOX-1.6 platform common)
