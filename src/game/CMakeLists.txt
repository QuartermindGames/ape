set(GAME_SHARED_FILES
        private/game.c
        private/game_client.c
        private/game_client_actions.c
        private/game_entity.c
        private/game_language.c
        private/game_menu.c
        private/game_server.c
        private/game_team.c
        private/game_test.c

        private/ai/ai_brain.c

        private/components/component_camera.c
        private/components/component_collision.c
        private/components/component_health.c
        private/components/component_inventory.c
        private/components/component_movement.c

        private/entities/qm1/qm1_entity_airship.c
        private/entities/qm1/qm1_entity_pawn.c
        private/entities/qm1/qm1_entity_player.c

        private/entities/qm2/qm2_entity_creature.c

        private/entities/entity_ambience_manager.c
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
# QM1
#############################################
set(SS1_SOURCES
        ${GAME_SHARED_FILES}

        private/nihlexa/nihlexa.c
        private/nihlexa/nihlexa_actions.c
        private/nihlexa/nihlexa_menu.c
        private/nihlexa/nihlexa_menu_hud.c
        private/nihlexa/nihlexa_mission.c
        private/nihlexa/nihlexa_name_generator.c
        private/nihlexa/nihlexa_world.c
)

add_executable(SS1 WIN32 ${SS1_SOURCES} ../shells/launcher_sdl2/launcher.c)
target_link_libraries(SS1 ${GAME_LINK_LIBS})
target_compile_definitions(SS1 PRIVATE -DGAME_SS1 GAME_QM2)
target_include_directories(SS1 PRIVATE private)
set_target_properties(SS1 PROPERTIES FOLDER "Games")

add_executable(SS1Forge WIN32 ${SS1_SOURCES})
target_link_libraries(SS1Forge Forge ${GAME_LINK_LIBS})
target_compile_definitions(SS1Forge PRIVATE -DGAME_SS1 GAME_QM2)
target_include_directories(SS1Forge PRIVATE private)
set_target_properties(SS1Forge PROPERTIES FOLDER "Games")

add_library(ape-game OBJECT
        ${SS1_SOURCES}
)
target_compile_definitions(ape-game PRIVATE -DGAME_SS1 GAME_QM2)
target_include_directories(ape-game PRIVATE private)
target_link_libraries(ape-game ${GAME_LINK_LIBS})
set_target_properties(ape-game PROPERTIES FOLDER "Engine")

#############################################
# Gateway
#############################################
set(QM2_SOURCES
        ${GAME_SHARED_FILES}

        private/gateway/gateway.c
        private/gateway/gateway_hud.c
        private/gateway/gateway_menu.c
)

add_executable(gateway WIN32 ${QM2_SOURCES} ../shells/launcher_sdl2/launcher.c)
target_link_libraries(gateway ${GAME_LINK_LIBS})
target_compile_definitions(gateway PRIVATE GAME_QM2)
target_include_directories(gateway PRIVATE private)
set_target_properties(gateway PROPERTIES FOLDER "Games")

add_executable(gateway-forge WIN32 ${QM2_SOURCES})
target_link_libraries(gateway-forge Forge ${GAME_LINK_LIBS})
target_compile_definitions(gateway-forge PRIVATE GAME_QM2)
target_include_directories(gateway-forge PRIVATE private)
set_target_properties(gateway-forge PROPERTIES FOLDER "Games")
