add_executable(cook
        cook.c
        cook_model.c
        cook_world.c
        cook_world_qmap.c

        model/model_md2.c
        model/model_obj.c
        model/model_psk.c
        model/model_smd.c
)

target_link_libraries(cook
        ape-common
        ape-core
)
