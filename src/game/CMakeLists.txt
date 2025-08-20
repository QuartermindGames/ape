set(GAME_SHARED_FILES
        private/shared/physics/physics_rope.c
        private/shared/game.c
        private/shared/game_client.c
        private/shared/game_client_actions.c
        private/shared/game_entity.c
        private/shared/game_language.c
        private/shared/game_menu.c
        private/shared/game_server.c
        private/shared/game_team.c
        private/shared/game_test.c

        #############################################

        private/shared/ai/ai_brain.c

        private/shared/components/component_collision.c
        private/shared/components/component_health.c
        private/shared/components/component_inventory.c
        private/shared/components/component_movement.c

        private/shared/entities/entity_player_spawn.c
        private/shared/entities/entity_portal.c
        private/shared/entities/entity_rope.c
        private/shared/entities/entity_trigger.c

        private/shared/integrations/discord/discord.c

        private/shared/menu/menu_chat.c
        private/shared/menu/menu_pie.c
        private/shared/menu/menu_splash.c
)

include_directories(public/)

if (MINGW)
    set(GAME_LINK_LIBS
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
            plcore
            plgraphics
            ApeCommon
            ApeCore
    )
else ()
    set(GAME_LINK_LIBS
            plcore
            plgraphics
            ApeCommon
            ApeCore
    )
endif ()

#############################################
# SS1
#############################################
set(SS1_SOURCES
        ${GAME_SHARED_FILES}

        private/ss1/entities/entity_airship.c
        private/ss1/entities/entity_pawn.c
        private/ss1/entities/entity_player.c

        private/ss1/menu/menu.c

        private/ss1/ss1_actions.c
        private/ss1/ss1_game.c
        private/ss1/ss1_mission.c
        private/ss1/ss1_name_generator.c
)

add_executable(SS1 WIN32 ${SS1_SOURCES} ../shells/launcher_sdl2/launcher.c)
target_link_libraries(SS1 ${GAME_LINK_LIBS})
target_compile_definitions(SS1 PRIVATE -DGAME_SS1)
target_include_directories(SS1 PRIVATE private)
set_target_properties(SS1 PROPERTIES FOLDER "Shells")

add_executable(SS1Forge WIN32 ${SS1_SOURCES})
target_link_libraries(SS1Forge Forge ${GAME_LINK_LIBS})
target_compile_definitions(SS1Forge PRIVATE -DGAME_SS1)
target_include_directories(SS1Forge PRIVATE private)
set_target_properties(SS1Forge PROPERTIES FOLDER "Shells")
