add_executable(cook
        cook.c
        cook_world.c

        model/format_obj.c
        model/format_qc.c
        model/format_smd.c
)

target_link_libraries(cook
        ApeCommon
        ApeCore
)
