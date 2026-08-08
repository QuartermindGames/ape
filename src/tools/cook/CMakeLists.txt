add_executable(cook
        cook.c
        cook_model.c
        cook_world.c
        cook_world_qmap.c

        model/model_obj.c
        model/model_smd.c
)
set_target_properties(cook PROPERTIES FOLDER "Tools")
target_link_libraries(cook ape-aux ape-core)
