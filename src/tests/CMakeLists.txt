add_executable(ape-tests tests.c)

set_target_properties(ape-tests PROPERTIES FOLDER "Utilities")

# Setup link libraries
if (NOT UNIX AND NOT MSVC)
    target_link_libraries(ape-tests mingw32)
endif ()
target_link_libraries(ape-tests plcore yin-common yin-node)
