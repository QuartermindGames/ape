add_executable(matgen WIN32 matgen.c)
set_target_properties(matgen PROPERTIES FOLDER "Tools")
target_link_libraries(matgen plcore ape-aux acm)
target_include_directories(matgen PRIVATE ../../core/public)
