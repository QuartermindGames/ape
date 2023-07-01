add_executable(m2w
        map2world.c
        )

set_target_properties(m2w PROPERTIES FOLDER "Utilities")

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(m2w mingw32)
endif ()
target_link_libraries(m2w plcore plgraphics ape-common ape-node)
