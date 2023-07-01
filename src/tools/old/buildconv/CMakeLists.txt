add_executable(buildconv
        buildconv.c)

set_target_properties(buildconv PROPERTIES FOLDER "Utilities")

# Setup link libraries
if (NOT UNIX)
    target_link_libraries(buildconv mingw32)
endif ()
target_link_libraries(buildconv plcore plgraphics plmodel ape-common ape-node)
