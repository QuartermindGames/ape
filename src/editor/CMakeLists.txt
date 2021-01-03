# Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>

project(FoxEd)

add_executable(FoxEd
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
        #mru.cpp
        #mru.h
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
        z.h)

target_include_directories(FoxEd PRIVATE ../3rdparty/fox-1.6.57/include/)
target_link_libraries(FoxEd libfox-1.6 libglew)
