add_executable(cook
        cook.c
        cook_model.c
        cook_world.c

        model/model_how_pc.c
        model/model_obj.c
        model/model_qc.c
        model/model_rfm.c
        model/model_smd.c
)

target_link_libraries(cook
        ApeCommon
        ApeCore
)
