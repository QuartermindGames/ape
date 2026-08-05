add_library(io_model OBJECT
        private/io_model.c
        private/io_model_smd.c
)

target_include_directories(io_model PRIVATE private)
target_link_libraries(io_model qm-os plcore)

set_target_properties(io_model PROPERTIES FOLDER "Engine")
