add_executable(graft WIN32
        graft.c
)

target_link_libraries(graft
        ape-core
        ape-game
)

set_target_properties(graft PROPERTIES FOLDER "Tools")
