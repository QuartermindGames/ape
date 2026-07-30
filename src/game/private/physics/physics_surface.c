// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Surface definition stuff -
//			I have no idea if this makes sense to be a "physics" thing...
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_string.h"

#include "physics.h"

typedef struct GamePhysicsSurface
{
	char *key;
} GamePhysicsSurface;

static GamePhysicsSurface *surfaces;
static uint8_t             numSurfaces;

void game_physics_surface_initialize()
{
	static constexpr char SURFACES_PATH[] = "scripts/surfaces" ACM_DEFAULT_EXTENSION;

	AcmBranch *root = com_acm_load_file( SURFACES_PATH, "surfaces" );
	if ( root == nullptr )
	{
		game_warning_( "Failed to fetch surfaces config (%s)!\n", SURFACES_PATH );
		return;
	}

	numSurfaces = acm_get_num_of_children( root );
	if ( numSurfaces == 0 )
	{
		game_warning_( "No surfaces defined in surfaces file (%s)!\n", SURFACES_PATH );
		return;
	}

	surfaces = APE_MEMORY_NEW_C( GamePhysicsSurface, numSurfaces );

	GamePhysicsSurface *surface = surfaces;
	ACM_ITERATE_BRANCH( root, i )
	{
		surface->key = qm_os_string_alloc( "%s", acm_get_string( i, "key", "default" ) );
		game_debug_( "Surface %u registered as \"%s\"\n", surface - surfaces, surface->key );

		surface++;
	}
}

void game_physics_surface_shutdown()
{
	for ( unsigned int i = 0; i < numSurfaces; ++i )
	{
		qm_os_memory_free( surfaces[ i ].key );
	}

	qm_os_memory_free( surfaces );
}

const char *game_physics_surface_get_key( uint8_t index )
{
	if ( index >= numSurfaces )
	{
		return nullptr;
	}

	return surfaces[ index ].key;
}

uint8_t game_physics_surface_get_num()
{
	return numSurfaces;
}
