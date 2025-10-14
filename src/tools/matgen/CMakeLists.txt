add_executable(matgen WIN32 matgen.c)
target_link_libraries(matgen plcore ape-common acm)
target_include_directories(matgen PRIVATE ../../core/public)