add_executable(matgen WIN32 matgen.c)
target_link_libraries(matgen plcore ApeCommon ApeNode)
target_include_directories(matgen PRIVATE ../../core/public)