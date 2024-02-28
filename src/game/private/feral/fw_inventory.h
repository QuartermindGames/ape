// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "fw_game.h"

#define FW_INVENTORY_MAX_SLOTS 32

typedef struct FWInventoryItem
{
	char *name;// name of the item
	char *description;
	char *spawnName;
	ApeMaterial *icon;
	unsigned int quantity;
	float weight;
} FWInventoryItem;

typedef struct FWInventory
{
	FWInventoryItem items[ FW_INVENTORY_MAX_SLOTS ];
	unsigned int numItems;
} FWInventory;
