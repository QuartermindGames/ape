find_library(SMACKER_LIBRARY NAMES smacker)

if (APE_OPTION_TOOLS)
    set(APE_SOURCE_EDITOR
            private/editor/modes/mode_polygon.c
            private/editor/modes/mode_transform.c
            private/editor/editor.c
            private/editor/editor_grid.c
            private/editor/editor_light.c
            private/editor/editor_selection.c
            private/editor/editor_ui.c
    )

    # This is sort of gross - just use the fallback per ISPC
    add_library(ispc_texcomp OBJECT ../3rdparty/ispc_texcomp/ispc_fallback.cpp)
    target_compile_definitions(ispc_texcomp PRIVATE ISPC_FALLBACK_MODE)
    target_include_directories(ispc_texcomp PUBLIC ../3rdparty/ispc_texcomp)
endif ()

add_library(ape-core STATIC
        # Audio
        private/audio/audio.c
        private/audio/audio_openal.c
        private/audio/audio_format_vorbis.c
        private/audio/audio_format_wav.c
        private/audio/audio_xaudio2.cpp

        private/camera/camera.c

        # Console
        private/console/console.c
        private/console/console_client.c
        private/console/console_cmd.c
        private/console/console_var.c

        # Scene Node System
        private/node/node_brush.c
        private/node/node_entity.c
        private/node/node_light.c
        private/node/node_room.c

        # Networking
        private/net/net.c

        private/model/model.c

        private/world/world.c
        private/world/world_node.c

        private/client/client.c
        private/client/client_input.c
        private/client/client_gui.c

        private/renderer/material/material.c
        private/renderer/material/material_animator.c
        private/renderer/material/material_shaders.c
        private/renderer/post/post.c
        private/renderer/post/post_bloom.c
        private/renderer/post/post_dither.c
        private/renderer/post/post_dof.c
        private/renderer/post/post_fxaa.c
        private/renderer/renderer_batch.c
        private/renderer/renderer_decal.c
        private/renderer/renderer_flare.c
        private/renderer/renderer_particle.c
        private/renderer/renderer.c
        private/renderer/renderer_draw.c
        private/renderer/renderer_render_target.c
        private/renderer/renderer_texture.c
        private/renderer/renderer_viewport.c
        private/renderer/renderer_world.c
        private/renderer/renderer_sprite.c

        # Script
        private/script/script.c

        # GUI
        private/gui/gui.c
        private/gui/gui_draw.c
        private/gui/gui_font.c

        ${APE_SOURCE_EDITOR}

        # Memory
        private/memory/memory.c

        # Server
        private/server/server.c

        # Video
        private/video/video.c
        private/video/video_smk.c

        # Other
        private/ape.c
        private/ape_filesystem.c
        private/ape_interface_game.c
        private/ape_image.c
        private/ape_scheduler.c
        private/ape_test.c
)
add_dependencies(ape-core
        plcore
        plgraphics
        pl_gl_driver
)

if (SMACKER_LIBRARY)
    message("Found libsmacker, smk support will be enabled!")
    target_compile_definitions(ape-core PRIVATE APE_SUPPORT_SMACKER)
    target_link_libraries(ape-core ${SMACKER_LIBRARY})
endif ()

target_include_directories(ape-core PRIVATE
        private/
        ../game/public/
)
target_include_directories(ape-core PUBLIC
        public/
)

set_target_properties(ape-core PROPERTIES
        FOLDER "Engine"
        PREFIX ""
)

target_link_libraries(ape-core
        SDL3

        # qmfw libraries
        qm-os

        # platform libraries
        plcore
        plgraphics

        # ape libraries
        ape-aux
        acm

        # other
        OpenAL
        lua
)

####################################################################
# Tests
####################################################################

add_executable(ape-core_test test/core_test.c)
target_link_libraries(ape-core_test ape-core)

set_target_properties(ape-core_test PROPERTIES
        FOLDER "Tests"
        PREFIX ""
)

add_test(NAME ape-tests-core COMMAND ape-core_test)
