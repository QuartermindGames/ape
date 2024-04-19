cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_C_STANDARD 11)

set(GAME_SHARED_FILES
        private/shared/game.c
        private/shared/game_menu.c
        private/shared/game_menu_pie.c
        #############################################
        private/shared/ai/ai_brain.c

        private/shared/entities/entity_light.c
)
set(GAME_LINK_LIBS
        plcore
        plgraphics
        plmodel
        ApeCommon
        ApeCore
)

include_directories(public/)

if (MINGW)
    set(SHELL_LINK_LIBS
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
            ApeCore
    )
else ()
    set(SHELL_LINK_LIBS
            ApeCore
    )
endif ()

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

add_executable(Tox WIN32
        ${TOX_SOURCES}
        ../shells/launcher_sdl2/launcher.c
)
target_link_libraries(Tox ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS})
set_target_properties(Tox PROPERTIES FOLDER "Shells")

add_executable(ToxForge WIN32 ${TOX_SOURCES})
target_link_libraries(ToxForge ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS} Forge)
set_target_properties(ToxForge PROPERTIES FOLDER "Shells")

#############################################
# SS3
#############################################

set(SS3_SOURCES
        ${GAME_SHARED_FILES}
        private/ss3/ss3_game.c
        private/ss3/ss3_name_generator.c
        private/ss3/ss3_ui.c
)

add_executable(SS3 WIN32 ${SS3_SOURCES} ../shells/launcher_sdl2/launcher.c)
target_link_libraries(SS3 ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS})
set_target_properties(SS3 PROPERTIES FOLDER "Shells")

add_executable(SS3Forge WIN32 ${SS3_SOURCES})
target_link_libraries(SS3Forge ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS} Forge)
set_target_properties(SS3Forge PROPERTIES FOLDER "Shells")

#############################################
