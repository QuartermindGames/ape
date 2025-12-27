// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "world.h"
#include "renderer/renderer.h"

ApeWorld *ape_world_create( void )
{
	ApeWorld *world = QM_OS_MEMORY_NEW( ApeWorld );
	ape_world_node_setup_( &world->base, nullptr, APE_WORLD_NODE_TYPE_ROOT, nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );

	world->entities = PlCreateLinkedList();
	if ( world->entities == nullptr )
	{
		ape_console_error_( true, "Failed to create world entity list: %s\n", PlGetError() );
	}

	world->roomLookup = PlCreateHashTable();
	if ( world->roomLookup == nullptr )
	{
		ape_console_error_( true, "Failed to create world room lookup: %s\n", PlGetError() );
	}

	return world;
}

void ape_world_destroy_( void *data, ApeWorldNode *parent )
{
	ApeWorld *self = data;
	if ( self == nullptr )
	{
		return;
	}

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
		}
		PlDestroyVectorArray( self->materials );
		self->materials = nullptr;
	}

	PlDestroyLinkedList( self->entities );
	PlDestroyHashTable( self->roomLookup );

	qm_os_memory_free( self );
}

void ape_world_spawn_entities_( ApeWorld *self )
{
	ApeEntity *entity;
	COM_ITERATE_LINKED_LIST( entity, self->entities, i )
	{
		ape_entity_spawn( entity );
	}
}

void ape_world_tick_entities_( ApeWorld *self, double delta )
{
	ApeEntity *entity;
	COM_ITERATE_LINKED_LIST( entity, self->entities, i )
	{
		ape_entity_tick( entity, delta );
	}
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

static void on_attach_child( void *self, ApeWorldNode *child )
{
	if ( child->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		ape_console_warning_( "Attached a node other than a room to the root!\n" );
		return;
	}

	const char *path = ape_room_get_path( ( ApeRoom * ) child );
	if ( path == nullptr || *path == '\0' )
	{
		ape_console_warning_( "Attached a room with no path!\n" );
		return;
	}

	if ( PlInsertHashTableNode( ( ( ApeWorld * ) self )->roomLookup, path, strlen( path ), ( ApeRoom * ) child ) == nullptr )
	{
		ape_console_warning_( "Attempted to add duplicate room (%s)!\n", path );
		return;
	}

	ape_console_verbose_( "Added \"%s\" to world lookup\n", path );
}

static void on_dettach_child( void *self, ApeWorldNode *child )
{
	if ( child->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		return;
	}

	const char *path = ape_room_get_path( ( ApeRoom * ) child );
	if ( path == nullptr || *path == '\0' )
	{
		return;
	}

	PLHashTableNode *node = PlLookupHashTableNode( ( ( ApeWorld * ) self )->roomLookup, path, strlen( path ) );
	if ( node == nullptr )
	{
		ape_console_warning_( "Attempted to remove a room that wasn't in the lookup list (%s)!\n", path );
		return;
	}

	PlDestroyHashTableNode( node );

	ape_console_verbose_( "Removed \"%s\" from world lookup\n", path );
}

ApeRoom *ape_world_get_room_by_path( ApeWorld *self, const char *path )
{
	return PlLookupHashTableUserData( self->roomLookup, path, strlen( path ) );
}

ApeBrushFace *ape_world_get_tagged_surface( ApeWorld *self, const char *path )
{
	const char *seperator = strrchr( path, ':' );
	if ( seperator == nullptr )
	{
		ape_console_warning_( "Failed to find seperator in given path (%s), invalid tag name?\n", path );
		return nullptr;
	}

	PLPath roomPath;
	PlSetupPath( roomPath, true, "%s", path );
	roomPath[ seperator - path ] = '\0';

	ApeRoom *room = ape_world_get_room_by_path( self, roomPath );
	if ( room == nullptr )
	{
		ape_console_warning_( "Failed to get room by path (%s)!\n", path );
		return nullptr;
	}

	const char   *surfaceName = seperator + 1;
	ApeBrushFace *face        = ape_room_get_tagged_surface( room, surfaceName );
	if ( face == nullptr )
	{
		ape_console_warning_( "Failed to get tagged surface (%s)!\n", surfaceName );
		return nullptr;
	}

	return face;
}

ApeBrushFace **ape_world_get_tagged_surfaces( ApeWorld *self, unsigned int *numDst )
{
	// first determine how many there are
	ApeRoom     *room;
	unsigned int numTaggedSurfaces = 0;
	COM_ITERATE_HASHED_LIST( room, self->roomLookup, i )
	{
		numTaggedSurfaces += PlGetNumHashTableNodes( room->taggedSurfaceLookup );
	}

	if ( numTaggedSurfaces == 0 )
	{
		*numDst = 0;
		return nullptr;
	}

	// and now allocate and populate the list
	unsigned int   faceIndex = 0;
	ApeBrushFace **faces     = QM_OS_MEMORY_NEW_( ApeBrushFace *, numTaggedSurfaces );
	COM_ITERATE_HASHED_LIST( room, self->roomLookup, i )
	{
		ApeBrushFace *face;
		COM_ITERATE_HASHED_LIST( face, room->taggedSurfaceLookup, j )
		{
			faces[ faceIndex++ ] = face;
		}
	}

	*numDst = numTaggedSurfaces;

	return faces;
}

void ape_register_world_console_variables_( void )
{
	PlRegisterConsoleVariable( "world.skipDraw", "Toggle rendering of world.", "false", PL_VAR_BOOL, &ape_config_.world.skipDraw, nullptr, false );
	PlRegisterConsoleVariable( "world.skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, &ape_config_.world.skipPortals, nullptr, false );
	PlRegisterConsoleVariable( "world.showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, &ape_config_.world.showAllRooms, nullptr, false );
	PlRegisterConsoleVariable( "world.showPortals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.world.showPortals, nullptr, false );
	PlRegisterConsoleVariable( "world.sortLights", "Sort lights before drawing world.", "false", PL_VAR_BOOL, &ape_config_.world.sortLights, nullptr, false );
	PlRegisterConsoleVariable( "world.showNodeVolumes", "Shows bounding volumes of all nodes currently in the scene.", "false", PL_VAR_BOOL, &ape_config_.world.showNodeVolumes, nullptr, false );
	PlRegisterConsoleVariable( "world.gravityModifier.x", "Modifies the given level of gravity.", "0.0", PL_VAR_F32, &ape_config_.world.gravityModifier.x, nullptr, false );
	PlRegisterConsoleVariable( "world.gravityModifier.y", "Modifies the given level of gravity.", "0.0", PL_VAR_F32, &ape_config_.world.gravityModifier.y, nullptr, false );
	PlRegisterConsoleVariable( "world.gravityModifier.z", "Modifies the given level of gravity.", "0.0", PL_VAR_F32, &ape_config_.world.gravityModifier.z, nullptr, false );
}

const ApeWorldNodeClass ape_rootClass = {
        .identifier = "root",
        .magic      = QM_OS_MAGIC_TO_NUM( 'W', 'L', 'D', ' ' ),

        .destroy = ape_world_destroy_,

        .onAttachChild  = on_attach_child,
        .onDettachChild = on_dettach_child,

        .flags = APE_WORLD_NODE_CLASS_FLAG_NO_EDITOR,
};
