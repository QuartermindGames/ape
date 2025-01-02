// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_filesystem.h>

#include "ape_private.h"
#include "world.h"
#include "renderer/renderer.h"

void ape_world_set_global_defaults( ApeWorld *level )
{
	level->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

ApeWorld *ape_world_create( void )
{
	ApeWorld *world = PL_NEW( ApeWorld );
	ape_world_node_setup_( &world->base, nullptr, APE_WORLD_NODE_TYPE_ROOT, nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );

	ape_world_set_global_defaults( world );

	return world;
}

void ape_world_destroy_( void *data, ApeWorldNode *parent )
{
	ApeWorld *self = data;
	if ( self == nullptr )
	{
		return;
	}

	ape_clear_camera_visibility_lists_();

	if ( self->materials != nullptr )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( self->materials ); ++i )
		{
			ApeMaterial *material = PlGetVectorArrayElementAt( self->materials, i );
			if ( material == nullptr )
			{
				continue;
			}

			ape_material_release( material );
			material = nullptr;
		}
		PlDestroyVectorArray( self->materials );
		self->materials = nullptr;
	}

	PlDestroyVectorArray( self->rooms );
	self->rooms = nullptr;
}

void ape_world_spawn_entities_( ApeWorld *world )
{
}

ApeRoom *ape_world_get_first_room_( ApeWorld *world )
{
	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, APE_WORLD_NODE( world )->children, i )
	{
		if ( child->type == APE_WORLD_NODE_TYPE_ROOM )
		{
			return ( ApeRoom * ) child;
		}
	}

	return nullptr;
}

void ape_register_world_console_variables_( void )
{
	PlRegisterConsoleVariable( "world.skipDraw", "Toggle rendering of world.", "false", PL_VAR_BOOL, &ape_config_.world.skipDraw, nullptr, false );
	PlRegisterConsoleVariable( "world.skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, &ape_config_.world.skipPortals, nullptr, false );
	PlRegisterConsoleVariable( "world.showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, &ape_config_.world.showAllRooms, nullptr, false );
	PlRegisterConsoleVariable( "world.showPortals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.world.showPortals, nullptr, false );
	PlRegisterConsoleVariable( "world.sortLights", "Sort lights before drawing world.", "false", PL_VAR_BOOL, &ape_config_.world.sortLights, nullptr, false );
	PlRegisterConsoleVariable( "world.showNodeVolumes", "Shows bounding volumes of all nodes currently in the scene.", "false", PL_VAR_BOOL, &ape_config_.world.showNodeVolumes, nullptr, false );
}

const ApeWorldNodeClass ape_rootClass = {
        .identifier      = "root",
        .magic           = PL_MAGIC_TO_NUM( 'W', 'L', 'D', ' ' ),
        .destroyFunction = ape_world_destroy_,
};
