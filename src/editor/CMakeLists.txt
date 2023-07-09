add_library(ApeEditor STATIC
        private/importers/models/importer_model.c
        private/importers/models/importer_model_gsmdl.c
        private/importers/models/importer_model_ocmsh.c
        private/importers/models/importer_model_q2md2.c
        private/editor.c
        private/editor_lightmapper.c
        private/editor_material_selector.c
        private/editor_world.c)

target_include_directories(ApeEditor PUBLIC public/)
target_compile_definitions(ApeEditor PUBLIC APE_EDITOR_ENABLED=1)

target_link_libraries(ApeEditor
        plcore plgraphics plmodel
        ApeCommon ApeCore)
