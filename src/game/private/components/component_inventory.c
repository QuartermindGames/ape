// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Handler for general entity movement.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "component_inventory.h"

static void *create_inventory()
{
	return QM_OS_MEMORY_NEW( GameInventoryComponent );
}

static void destroy_inventory( void *data )
{
	GameInventoryComponent *inventory = data;
	qm_os_memory_free( inventory );
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
