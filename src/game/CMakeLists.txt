set(GAME_SHARED_FILES
        private/game.c
        private/game_client.c
        private/game_entity.c
        private/game_language.c
        private/game_menu.c
        private/game_server.c
        private/game_team.c
        private/game_test.c

        private/ai/ai_brain.c

        private/client/client_actions.c

        private/components/component_camera.c
        private/components/component_collision.c
        private/components/component_health.c
        private/components/component_inventory.c
        private/components/component_movement.c

        private/entities/qm1/qm1_entity_pawn.c
        private/entities/qm1/qm1_entity_player.c

        private/entities/entity_ambience_manager.c
        private/entities/entity_decal.c
        private/entities/entity_path.c
        private/entities/entity_player_spawn.c
        private/entities/entity_portal.c
        private/entities/entity_rope.c
        private/entities/entity_terrain.c
        private/entities/entity_trigger.c

        private/integrations/discord/discord.c

        private/menu/menu.c
        private/menu/menu_chat.c
        private/menu/menu_compass.c
        private/menu/menu_pie.c
        private/menu/menu_splash.c

        private/physics/physics_rope.c

        private/rpg/rpg_component.c
)

include_directories(public/)

if (MINGW)
    set(GAME_LINK_LIBS
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive

            qm-os
            qm-math

            plcore
            plgraphics
            ape-aux
            ape-core
    )
else ()
    set(GAME_LINK_LIBS
            qm-os
            qm-math

            plcore
            plgraphics
            ape-aux
            ape-core
    )
endif ()

#############################################
# Nihlexa
#############################################
set(NIH_SOURCES
        ${GAME_SHARED_FILES}

        private/nihlexa/nihlexa.c
        private/nihlexa/nihlexa_actions.c
        private/nihlexa/nihlexa_client.c
        private/nihlexa/nihlexa_menu.c
        private/nihlexa/nihlexa_menu_hud.c
        private/nihlexa/nihlexa_world.c
)

add_executable(Nihlexa WIN32 ${NIH_SOURCES} ../shells/launcher_sdl2/launcher.c)
target_link_libraries(Nihlexa ${GAME_LINK_LIBS})
target_compile_definitions(Nihlexa PRIVATE -DGAME_SS1 GAME_QM2)
target_include_directories(Nihlexa PRIVATE private)
set_target_properties(Nihlexa PROPERTIES FOLDER "Games")

add_executable(NEditor WIN32 ${NIH_SOURCES})
target_link_libraries(NEditor Forge ${GAME_LINK_LIBS})
target_compile_definitions(NEditor PRIVATE -DGAME_SS1 GAME_QM2)
target_include_directories(NEditor PRIVATE private)
set_target_properties(NEditor PROPERTIES FOLDER "Games")

add_library(ape-game OBJECT
        ${NIH_SOURCES}
)
target_compile_definitions(ape-game PRIVATE -DGAME_SS1 GAME_QM2)
target_include_directories(ape-game PRIVATE private)
target_link_libraries(ape-game ${GAME_LINK_LIBS})
set_target_properties(ape-game PROPERTIES FOLDER "Engine")
