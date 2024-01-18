cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_C_STANDARD 11)

set(GAME_SHARED_FILES
        private/shared/game.c
        private/shared/game_entity_class_base.c
        private/shared/game_menu.c
        private/shared/game_menu_pie.c
        #############################################
        private/shared/ai/ai_brain.c
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
# FERAL / SS1
#############################################

set(FERAL_SOURCES
        ${GAME_SHARED_FILES}
        private/feral/menu/fw_menu.c
        private/feral/fw_building.c
        private/feral/fw_character.c
        private/feral/fw_component_stats.c
        private/feral/fw_game.c
        private/feral/fw_inventory.c
        private/feral/fw_name_generator.c
        private/feral/fw_terrain.c
        private/feral/fw_weapon.c
)

add_executable(FeralShell WIN32
        ${FERAL_SOURCES}
        ../shells/launcher_sdl2/launcher.c
)
target_link_libraries(FeralShell ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS})
set_target_properties(FeralShell PROPERTIES FOLDER "Shells")

add_executable(FeralForgeShell WIN32 ${FERAL_SOURCES})
target_link_libraries(FeralForgeShell ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS} Forge)
set_target_properties(FeralForgeShell PROPERTIES FOLDER "Shells")

#############################################
# DETOX / SS2
#############################################

set(DETOX_SOURCES
        ${GAME_SHARED_FILES}
        private/detox/entities/light_night.c
        private/detox/ui/tox_ui.c
        private/detox/ui/tox_ui_draw.c
        private/detox/tox_character.c
        private/detox/tox_game.c
        private/detox/tox_quest.c
        private/detox/tox_world.c
)

add_executable(DetoxShell WIN32
        ${DETOX_SOURCES}
        ../shells/launcher_sdl2/launcher.c
)
target_link_libraries(DetoxShell ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS})
set_target_properties(DetoxShell PROPERTIES FOLDER "Shells")

add_executable(DetoxForgeShell WIN32 ${DETOX_SOURCES})
target_link_libraries(DetoxForgeShell ${SHELL_LINK_LIBS} ${GAME_LINK_LIBS} Forge)
set_target_properties(DetoxForgeShell PROPERTIES FOLDER "Shells")

#############################################
