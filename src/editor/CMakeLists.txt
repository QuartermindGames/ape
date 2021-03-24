# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

project(editor)

if (UNIX)
    set(UNIX_ADDITIONAL)
    file(GLOB UNIX_ADDITIONAL
            findfirst.c
            findfirst.h
            spec.c
            spec.h)
endif ()

add_executable(editor
        brush.cpp
        brush.h
        bspfile.h
        camera.cpp
        camera.h
        cmdlib.cpp
        cmdlib.h
        csg.cpp
        drag.cpp
        eclass.cpp
        entity.cpp
        entity.h
        entityw.h
        Layer.cpp
        Layer.h
        lbmlib.cpp
        lbmlib.h
        MainWindow.h
        map.cpp
        map.h
        mathlib.cpp
        mathlib.h
        parse.cpp
        parse.h
        points.cpp
        qe3.cpp
        qe3.h
        qedefs.h
        qfiles.h
        resource.h
        select.cpp
        select.h
        textures.cpp
        textures.h
        Utilities.cpp
        vertsel.cpp
        view.h
        Viewport.cpp
        Viewport.h
        win_cam.cpp
        win_dlg.cpp
        win_ent.cpp
        win_main.cpp
        win_qe3.cpp
        win_xy.cpp
        win_z.cpp
        xy.cpp
        xy.h
        z.cpp
        z.h

        # Mostly for dir search functions
        ${UNIX_ADDITIONAL}
        )

if (NOT UNIX)
    target_include_directories(editor PRIVATE "../3rdparty/fox-1.6.57/include/" "glew/include/")
    target_link_directories(editor PRIVATE "glew/lib/Release/x64/" "../3rdparty/fox-1.6.57/lib/")
    target_link_libraries(editor glew32s)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(platform PRIVATE -static -static-libstdc++ -static-libgcc)
    endif ()
else ()
    target_link_libraries(editor GLEW GL GLU)
endif ()

target_link_libraries(editor platform common FOX-1.6)
