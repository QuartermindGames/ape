add_executable(ApeMap2World WIN32
        map2world.c
)
set_target_properties(ApeMap2World PROPERTIES FOLDER "Utilities")
target_link_libraries(ApeMap2World
        plcore
        ApeCommon ApeNode ApeCore
)
