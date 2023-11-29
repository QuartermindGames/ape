cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_C_STANDARD 11)

set(GAME_SHARED_FILES
        private/game.c
        private/game_entity_class_base.c
        private/game_menu.c
        private/game_menu_pie.c

        private/ai/ai_brain.c
)
set(GAME_LINK_LIBS
        plcore
        plgraphics
        plmodel
        ApeCommon
        ApeCore
)
set(GAME_INCLUDE_DIRS
        public/
)

#############################################
# Target Games

set(GAME_USER_TARGET "mag" CACHE STRING "Game to build.")

set(GAME_TARGET_MAG
        private/mag/mag_creature.c
        private/mag/mag_game.c
        private/mag/mag_tile_editor.c
        private/mag/mag_world.c
)
set(GAME_TARGET_FERAL
        private/fw/menu/fw_menu.c
        private/fw/fw_building.c
        private/fw/fw_character.c
        private/fw/fw_component_stats.c
        private/fw/fw_game.c
        private/fw/fw_inventory.c
        private/fw/fw_name_generator.c
        private/fw/fw_terrain.c
        private/fw/fw_weapon.c
)
set(GAME_TARGET_DETOX
        private/detox/entities/light_night.c

        private/detox/tox_character.c
        private/detox/tox_game.c
        private/detox/tox_quest.c
        private/detox/tox_world.c
)

#todo: need a better way of setting targets...
if (${GAME_USER_TARGET} STREQUAL "mag")
    set(GAME_TARGET ${GAME_TARGET_MAG})
elseif (${GAME_USER_TARGET} STREQUAL "feral")
    set(GAME_TARGET ${GAME_TARGET_FERAL})
elseif (${GAME_USER_TARGET} STREQUAL "detox")
    set(GAME_TARGET ${GAME_TARGET_DETOX})
else ()
    message("No valid game target specified!")
endif ()

#############################################

add_library(ApeGame STATIC
        ${GAME_SHARED_FILES}
        ${GAME_TARGET}
)

set_target_properties(ApeGame PROPERTIES PREFIX "")

# Setup link libraries
if (NOT UNIX AND NOT MSVC)
    target_link_libraries(ApeGame mingw32)
endif ()

target_link_libraries(ApeGame ${GAME_LINK_LIBS})
target_include_directories(ApeGame PUBLIC public/)
