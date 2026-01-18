// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: A marker indicating where a player can spawn.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "entity_player_spawn.h"

static PLLinkedList *playerSpawnPoints;

PLLinkedList *game_player_spawn_get_spawn_points()
{
	return playerSpawnPoints;
}

static void *create_player_spawn( ApeEntity *self, AcmBranch *properties )
{
	GamePlayerSpawnEntity *spawnEntity = QM_OS_MEMORY_NEW( GamePlayerSpawnEntity );

	if ( playerSpawnPoints == nullptr )
	{
		playerSpawnPoints = PlCreateLinkedList();
	}

	spawnEntity->listNode = PlInsertLinkedListNode( playerSpawnPoints, self );

	return spawnEntity;
}

static void destroy_player_spawn( ApeEntity *self )
{
	GamePlayerSpawnEntity *spawnEntity = PLAYER_SPAWN_ENTITY( self );
	assert( spawnEntity != nullptr );

	PlDestroyLinkedListNode( spawnEntity->listNode );
	if ( playerSpawnPoints != nullptr && PlGetNumLinkedListNodes( playerSpawnPoints ) == 0 )
	{
		PlDestroyLinkedList( playerSpawnPoints );
		playerSpawnPoints = nullptr;
	}

	qm_os_memory_free( spawnEntity );
}

static void entity_player_spawn_on_draw_editor_( ApeEntity *self, const bool isSelected )
{
	PLCollisionAABB bounds = {};
	bounds.origin          = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	bounds.maxs            = QM_MATH_VECTOR3F( 16.0f, 72.0f, 16.0f );
	bounds.mins            = QM_MATH_VECTOR3F( -16.0f, 0.0f, -16.0f );

	ape_draw_debug_aabb( &bounds, isSelected ? APE_EDITOR_COLOUR_SELECT_BOUNDS : PL_COLOUR_BLUE );
}

static ApePropertyEnum teamsEnum[] = {
        {"Red",   0},
        {"Green", 1},
        {"Blue",  2},
        {"Pink",  3},
};

static ApeProperty spawnProperties[] = {
        APE_PROPERTY_ENUM( "Team", "Team index this spawn is associated with.", GamePlayerSpawnEntity, team, teamsEnum ),
};

ApeEntityClassDefinition game_playerSpawnEntityClass_ = {
        .name            = GAME_PLAYER_SPAWN_CLASS_NAME,
        .description     = "Creates a marker indicating where the player can spawn.",
        .createFunction  = create_player_spawn,
        .destroyFunction = destroy_player_spawn,

        .onDrawEditor = entity_player_spawn_on_draw_editor_,

        .properties    = spawnProperties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( spawnProperties ),
};
