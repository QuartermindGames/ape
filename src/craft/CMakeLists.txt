####################################################################
# Craft : Qt-based frontend for editing ApeTech (to replace Forge)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(ape-craft STATIC
        private/editors/editor_room.cpp
        private/craft.cpp
        private/craft_project.cpp)

set_target_properties(ape-craft PROPERTIES FOLDER "Engine")

target_link_libraries(ape-craft
        qm-os
        qm-math
        plcore
        acm

        Qt6::Core
        Qt6::Widgets)
