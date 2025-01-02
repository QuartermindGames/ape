set(YIN_CORE_CLIENT
        private/client/ape_client.c
        private/client/client_console.c
        private/client/ape_client_input.c
        private/client/ape_client_gui.c

        private/renderer/material/material.c
        private/renderer/material/material_shaders.c

        private/renderer/post/post_bloom.c
        private/renderer/post/post_fxaa.c
        private/renderer/post/post.c

        private/renderer/renderer_flare.c
        private/renderer/renderer_font.c
        private/renderer/renderer_particle.c
        private/renderer/renderer.c
        private/renderer/renderer_draw.c
        private/renderer/renderer_render_target.c
        private/renderer/renderer_texture.c
        private/renderer/renderer_viewport.c
        private/renderer/renderer_world.c
        private/renderer/renderer_sprite.c
)

set(APE_SOURCE_GUI
        private/gui/gui.c
        private/gui/gui_button.c
        private/gui/gui_desktop.c
        private/gui/gui_cursor.c
        private/gui/gui_draw.c
        private/gui/gui_font.c
        private/gui/gui_image.c
        private/gui/gui_panel.c
)

set(APE_SOURCE_EDITORS
        private/editor/editor.c
        private/editor/editor_grid.c
        private/editor/editor_light.c
        private/editor/editor_selection.c
        private/editor/editor_vector.c
)

add_library(ApeCore STATIC
        private/ape_console.c

        # Scene Node System
        private/node/node_brush.c
        private/node/node_camera.c
        private/node/node_entity.c
        private/node/node_light.c
        private/node/node_room.c

        # Networking
        private/net/net.c
        private/net/net_var.c

        private/ape_filesystem.c
        private/ape_interface_game.c
        private/ape_image.c
        private/ape_memory.c
        private/ape_scheduler.c
        private/ape_script.c
        private/ape_binary_serializer.c

        private/ape.c

        private/model/model.c

        private/world/world.c
        private/world/world_node.c

        ${YIN_CORE_CLIENT}
        private/audio/audio.c
        private/audio/audio_openal.c
        private/audio/audio_format_vorbis.c
        private/audio/audio_format_wav.c
        private/audio/audio_xaudio2.cpp
        ${APE_SOURCE_GUI}
        ${APE_SOURCE_EDITORS}

        private/legacy/actor.c
        private/legacy/actors/actor_player.c
        private/legacy/actors/sg_actors.c

        private/server/server.c

        private/ape_test.c
)
add_dependencies(ApeCore
        plcore
        plgraphics
        pl_gl_driver
)

target_include_directories(ApeCore PRIVATE
        private/
        ../game/public/
)
target_include_directories(ApeCore PUBLIC
        public/
)

set_target_properties(ApeCore PROPERTIES
        FOLDER "Engine"
        PREFIX ""
)

if (WIN32)
    target_link_libraries(ApeCore
            SDL2_dll)
else ()
    target_link_libraries(ApeCore
            SDL2
            openal)
endif ()

target_link_libraries(ApeCore
        plcore
        plgraphics
        ApeCommon
        acm
)
