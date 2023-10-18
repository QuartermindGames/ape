add_executable(pkgman
        # Game specific loaders

        pack_image.c
        pack_model.c
        pack_model_smd.c
        pkgman.c
        pkgman.h
        parser.c
        parser.h
        txc_compress_dxtn.c
        txc_dxtn.h)

#file(COPY pkgman_legal.txt DESTINATION ${CMAKE_HOME_DIRECTORY}/release/docs/)

set_target_properties(pkgman PROPERTIES FOLDER "Utilities")
target_link_libraries(pkgman plcore plmodel ApeCommon ApeNode)
