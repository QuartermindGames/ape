add_executable(cook
        cook.c
        cook_model_qc.c
        cook_model_smd.c
)

target_link_libraries(cook
        ApeCommon
        ApeCore
)
