// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Building logic for Feral Warfare.
// Author:  Mark E. Sowden

#include "fw_building.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void fw_building_tick( ApeEntity *self )
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeEntityClassDefinition fw_buildingClassDefinition = {
        .name = "fw_building",
        .description = "Building, that can either be constructed by a player, or placed in the environment.",
        .tickFunction = fw_building_tick,
};
