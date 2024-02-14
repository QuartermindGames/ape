// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "fw_game.h"

enum
{
	PL_BITFLAG( FW_BUILDING_FLAG_POWER, 0U ),
	PL_BITFLAG( FW_BUILDING_FLAG_WATER, 1U ),
};

typedef struct FWBuildingComponent
{
	uint16_t power;

	uint32_t inputs;
	uint32_t outputs;

	uint8_t numOccupants;
	uint8_t maxOccupants;
} FWBuildingComponent;

extern ApeEntityClassDefinition fw_buildingClassDefinition;
