set(GAME_SHARED_FILES
        private/shared/physics/physics_rope.c
        private/shared/game.c
        private/shared/game_client.c
        private/shared/game_menu.c
        private/shared/game_menu_pie.c
        private/shared/game_server.c
        #############################################
        private/shared/ai/ai_brain.c

        private/shared/entities/entity_light.c
)

include_directories(public/)

if (MINGW)
    set(GAME_LINK_LIBS
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
            plcore
            plgraphics
            plmodel
            ApeCommon
            ApeCore
    )
else ()
    set(GAME_LINK_LIBS
            plcore
            plgraphics
            plmodel
            ApeCommon
            ApeCore
    )
endif ()

#############################################
# SS1
#############################################
set(SS1_SOURCES
        ${GAME_SHARED_FILES}
        private/ss1/menu/ss1_menu.c
        private/ss1/ss1_game.c
        private/ss1/ss1_character.c
        private/ss1/ss1_name_generator.c
)

add_executable(SS1 WIN32 ${SS1_SOURCES} ../shells/launcher_sdl2/launcher.c)
target_link_libraries(SS1 ${GAME_LINK_LIBS})
set_target_properties(SS1 PROPERTIES FOLDER "Shells")

add_executable(SS1Forge WIN32 ${SS1_SOURCES})
target_link_libraries(SS1Forge Forge ${GAME_LINK_LIBS})
set_target_properties(SS1Forge PROPERTIES FOLDER "Shells")

#############################################
# DETOX / SS2
#############################################
set(TOX_SOURCES
        ${GAME_SHARED_FILES}
        private/detox/entities/light_night.c
        private/detox/ui/tox_ui.c
        private/detox/tox_character.c
        private/detox/tox_game.c
        private/detox/tox_quest.c
        private/detox/tox_world.c
)

add_executable(Tox WIN32 ${TOX_SOURCES} ../shells/launcher_sdl2/launcher.c)
target_link_libraries(Tox ${GAME_LINK_LIBS})
set_target_properties(Tox PROPERTIES FOLDER "Shells")

add_executable(ToxForge WIN32 ${TOX_SOURCES})
target_link_libraries(ToxForge Forge ${GAME_LINK_LIBS})
set_target_properties(ToxForge PROPERTIES FOLDER "Shells")
