add_library(io_model STATIC
        private/io_model.c
        private/io_model_smd.c
)

target_include_directories(io_model PRIVATE private)
target_link_libraries(io_model qm-os plcore)

set_target_properties(io_model PROPERTIES FOLDER "Engine")

####################################################################
# Tests
####################################################################

add_executable(ape-io_model_test test/io_test.c)
target_link_libraries(ape-io_model_test
        qm-os
        plcore
        ape-aux
        io_model)

set_target_properties(ape-io_model_test PROPERTIES
        FOLDER "Tests"
        PREFIX ""
)

add_test(NAME ape-tests-io-model COMMAND ape-io_model_test)
