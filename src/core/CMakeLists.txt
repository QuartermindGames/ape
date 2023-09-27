set(YIN_CORE_CLIENT
        private/client/ape_client.c
        private/client/ape_client_decal.c
        private/client/client_console.c
        private/client/ape_client_input.c
        private/client/ape_client_gui.c
        private/client/renderer/post/post_bloom.c
        private/client/renderer/post/post_fxaa.c
        private/client/renderer/post/post.c
        private/client/renderer/renderer_camera.c
        private/client/renderer/renderer_flare.c
        private/client/renderer/renderer_font.c
        private/client/renderer/renderer_particle.c
        private/client/renderer/renderer.c
        private/client/renderer/renderer_draw.c
        private/client/renderer/renderer_material.c
        private/client/renderer/ar_render_target.c
        private/client/renderer/renderer_texture.c
        private/client/renderer/renderer_viewport.c
        private/client/renderer/renderer_world.c
        private/client/renderer/renderer_shaders.c
        private/client/renderer/renderer_sprite.c)

set(APE_SOURCE_AUDIO
        private/client/audio/audio.c
        private/client/audio/audio_openal.c
        private/client/audio/audio_wav.c
        private/client/audio/audio_xaudio2.cpp
        private/client/audio/audio_xwb.c)

set(APE_SOURCE_GUI
        private/client/gui/gui.c
        private/client/gui/gui_button.c
        private/client/gui/gui_desktop.c
        private/client/gui/gui_cursor.c
        private/client/gui/gui_draw.c
        private/client/gui/gui_font.c
        private/client/gui/gui_image.c
        private/client/gui/gui_panel.c)

set(APE_SOURCE_EDITORS
        private/editor/editor.c
        private/editor/editor_material_selector.c
        private/editor/editor_world.c)

add_library(ApeCore STATIC
        private/ape_console.c

        # Entity System
        private/entity/entity.c
        private/entity/entity_component.c

        # Networking
        private/net/net.c
        private/net/net_var.c

        private/ape_filesystem.c
        private/ape_interface_game.c
        private/ape_image.c
        private/ape_memory_manager.c
        private/ape_model.c
        private/ape_scheduler.c
        private/ape_binary_serializer.c

        private/ape.c

        private/world/world.c
        private/world/world_deserialiser.c
        private/world/world_deserialiser_rf.c
        private/world/world_mesh.c
        private/world/world_room.c
        private/world/world_serialiser.c
        private/world/world_visibility.c

        ${YIN_CORE_CLIENT}
        ${APE_SOURCE_AUDIO}
        ${APE_SOURCE_GUI}
        ${APE_SOURCE_EDITORS}

        private/legacy/actor.c
        private/legacy/actors/actor_player.c
        private/legacy/actors/sg_actors.c

        private/server/server.c)
add_dependencies(ApeCore
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        )

target_include_directories(ApeCore PRIVATE
        private/
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
        plmodel
        ApeCommon
        ApeNode
        ApeGame
        )
