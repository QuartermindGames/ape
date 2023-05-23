set(YIN_CORE_CLIENT
        private/client/client.c
        private/client/client_console.c
        private/client/client_input.c
        private/client/client_gui.c
        private/client/audio/audio.c
        private/client/audio/audio_openal.c
        private/client/audio/audio_wav.c
        private/client/audio/audio_xaudio2.cpp
        private/client/audio/audio_xwb.c
        private/client/audio/micromod.c
        private/client/audio/sdlplayer.c
        private/client/gui/gui.c
        private/client/gui/gui_button.c
        private/client/gui/gui_cursor.c
        private/client/gui/gui_draw.c
        private/client/gui/gui_font.c
        private/client/gui/gui_image.c
        private/client/gui/gui_panel.c
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
        private/client/renderer/renderer_rendertarget.c
        private/client/renderer/renderer_texture.c
        private/client/renderer/renderer_viewport.c
        private/client/renderer/renderer_world.c
        private/client/renderer/renderer_scenegraph.c
        private/client/renderer/renderer_shaders.c
        private/client/renderer/renderer_sprite.c
        private/client/renderer/renderer_visibility.c)

add_library(yin-core STATIC
        private/core_console.c

        # Entity System
        private/entity/entity.c
        private/entity/entity_component.c
        private/entity/entity_manager.c

        # Networking
        private/net/net.c
        private/net/net_var.c

        private/core_filesystem.c
        private/core_game_interface.c
        private/core_image.c
        private/core_memory_manager.c
        private/core_model.c
        private/core_profiler.c
        private/core_scheduler.c
        private/core_binary_serializer.c
        private/world.c
        private/world_deserialiser.c
        private/world_mesh.c
        private/world_serialiser.c
        private/core.c

        private/editor/editor.c
        private/editor/editor_lightmapper.c
        private/editor/editor_material_selector.c
        private/editor/editor_world.c

        ${YIN_CORE_CLIENT}

        private/legacy/actor.c
        private/legacy/actors/actor_player.c
        private/legacy/actors/sg_actors.c

        private/server/server.c
        )
add_dependencies(yin-core
        plcore
        plgraphics
        plmodel
        pl_gl_driver
        )

target_include_directories(yin-core PRIVATE
        private/
        )
target_include_directories(yin-core PUBLIC
        public/
        )

set_target_properties(yin-core PROPERTIES
        FOLDER "Engine"
        PREFIX ""
        )

if(WIN32)
    target_link_libraries(yin-core
            SDL2_dll)
else()
    target_link_libraries(yin-core
            SDL2
            OpenAL)
endif()

target_link_libraries(yin-core
        plcore
        plgraphics
        plmodel
        yin-common
        yin-node
        yin-game
        )
