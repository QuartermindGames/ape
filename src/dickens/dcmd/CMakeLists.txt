add_executable(dcmd
        private/dcmd.c
)

set_target_properties(dcmd PROPERTIES FOLDER "Scripting/Dickens")

target_include_directories(dcmd
        PRIVATE private/
        PUBLIC public/
)
target_link_libraries(dcmd dlib)
