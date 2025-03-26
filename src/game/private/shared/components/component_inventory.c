// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Handler for general entity movement.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "component_inventory.h"

static void *create_inventory()
{
	return PL_NEW( GameInventoryComponent );
}

static void destroy_inventory( void *data )
{
	GameInventoryComponent *inventory = data;
	PL_DELETE( inventory );
}

static AcmBranch *serialize_inventory( void *ptr, AcmBranch *root )
{
	GameInventoryComponent *inventory = ptr;
	return root;
}

static void *deserialize_inventory( void *ptr, AcmBranch *root )
{
	GameInventoryComponent *inventory = ptr;
	return inventory;
}

ApeEntityComponentDefinition game_inventoryComponent_ = {
        .name = "inventory",

        .createFunction  = create_inventory,
        .destroyFunction = destroy_inventory,

        .serializeFunction   = serialize_inventory,
        .deserializeFunction = deserialize_inventory,
};
