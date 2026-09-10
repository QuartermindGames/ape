add_executable(cook
        cook.c
        cook_model.c

        model/model_smd.c
)
set_target_properties(cook PROPERTIES FOLDER "Tools")
target_link_libraries(cook ape-aux ape-core)
