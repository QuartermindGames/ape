// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "component_item.h"

static constexpr unsigned int GAME_INVENTORY_MAX_SLOTS = 64;

typedef enum GameInventorySlotType
{
	GAME_INVENTORY_SLOT_TYPE_NONE,
	GAME_INVENTORY_SLOT_TYPE_AMMO,
	GAME_INVENTORY_SLOT_TYPE_WEAPON,
} GameInventorySlotType;

typedef struct GameInventorySlotDescriptor
{
	const char           *iconPath;
	const char           *name;
	const char           *description;
	GameInventorySlotType type;
} GameInventorySlotDescriptor;

typedef struct GameInventorySlot
{
	const GameInventorySlotDescriptor *descriptor;

	ApeEntity   *item;
	unsigned int quantity;
} GameInventorySlot;

typedef struct GameInventoryComponent
{
	GameInventorySlot slots[ GAME_INVENTORY_MAX_SLOTS ];
} GameInventoryComponent;

unsigned int game_inventory_add_item_( GameInventoryComponent *self, ApeEntity *item, unsigned int quantity );
void         game_inventory_remove_item_( GameInventoryComponent *self, unsigned int slot );
