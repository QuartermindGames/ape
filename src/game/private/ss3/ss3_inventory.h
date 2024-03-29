// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ss3_game.h"

#define SS3_INVENTORY_MAX_SLOTS 32

typedef struct SS3InventoryItem
{
	char *name;// name of the item
	char *description;
	char *spawnName;
	ApeMaterial *icon;
	unsigned int quantity;
	float weight;
} SS3InventoryItem;

typedef struct SS3Inventory
{
	SS3InventoryItem items[ SS3_INVENTORY_MAX_SLOTS ];
	unsigned int numItems;
} SS3Inventory;
