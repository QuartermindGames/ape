// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: World state management.
// Author:  Mark E. Sowden

#include "nihlexa.h"

#include "entities/entity_player_spawn.h"
#include "entities/qm1/qm1_entity_player.h"

#include "integrations/integrations.h"

bool qm1_world_setup_()
{
	if ( ape_editor_is_active() )
	{
		return true;
	}

	qm1_state_.world = ape_world_create();
	if ( qm1_state_.world == nullptr )
	{
		game_error_( "Failed to create world!\n" );
		return false;
	}

	qm1_state_.spawnRoom = ( ApeRoom * ) ape_world_node_load( APE_WORLD_NODE( qm1_state_.world ), "rooms/game/cockpit.rom.n" );
	assert( ape_world_node_is_valid( APE_WORLD_NODE( qm1_state_.spawnRoom ), APE_WORLD_NODE_TYPE_ROOM ) );
	if ( qm1_state_.spawnRoom == nullptr )
	{
		game_error_( "Failed to load in spawn room!\n" );
		return false;
	}

	//TODO: temporary placeholder crap just to get this working...
	{
		qm1_state_.playRoom = ( ApeRoom * ) ape_world_node_load( APE_WORLD_NODE( qm1_state_.world ), "rooms/game/bog_busters.rom.n" );
		assert( ape_world_node_is_valid( APE_WORLD_NODE( qm1_state_.playRoom ), APE_WORLD_NODE_TYPE_ROOM ) );
		if ( qm1_state_.playRoom == nullptr )
		{
			game_error_( "Failed to load in play room!\n" );
			return false;
		}

		// load the player ship into the play room
		ApeWorldNode *shipNode = ape_world_node_load( APE_WORLD_NODE( qm1_state_.playRoom ), "prefabs/ships/ship_test.node" );
		if ( shipNode == nullptr )
		{
			game_error_( "Failed to load in player ship!\n" );
			return false;
		}

		ape_world_node_set_position( shipNode, &QM_MATH_VECTOR3F( 0.0f, 128.0f, 0.0f ) );
	}

	qm1_state_.camera = ape_create_camera( nullptr, nullptr, &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
	if ( qm1_state_.camera == nullptr )
	{
		game_error_( "Failed to create player camera!\n" );
		return false;
	}

	game_spawn_world( qm1_state_.world, qm1_state_.spawnRoom );

	// connect the viewport portal to the play room
	ApeBrushFace *srcFace = ape_room_get_tagged_surface( qm1_state_.spawnRoom, "viewport" );
	assert( srcFace != nullptr );
	snprintf( srcFace->destinationTag, sizeof( srcFace->destinationTag ), "rooms/game/bog_busters.rom.n:ship_portal_camera" );
	srcFace->flags |= APE_BRUSH_FACE_FLAG_PORTAL;

	return true;
}

static void spawn_player()
{
	// create all the characters for each spawn point
	QmOsLinkedList *playerSpawns = game_player_spawn_get_spawn_points();
	if ( playerSpawns == nullptr )
	{
		game_warning_( "Unable to spawn player entities, no spawn points!\n" );
		return;
	}

	// just get the first one - this used to be more sophisticated,
	// but because we're now just spawning the single player into a
	// "lobby" environment it doesn't matter

	QmOsLinkedListNode *node = qm_os_linked_list_get_front( playerSpawns );
	assert( node != nullptr );

	ApeEntity *spawnEntity = qm_os_linked_list_node_get_data( node );
	assert( spawnEntity != nullptr );

	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( spawnEntity ) );
	if ( room == nullptr )
	{
		game_warning_( "Encountered a player spawn without a room!\n" );
		return;
	}

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( spawnEntity ) );
	QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( spawnEntity ) );

	ApeEntity *player = ape_entity_create( APE_WORLD_NODE( room ), QM1_PLAYER_CLASS_NAME, "local_player", nullptr, &pos, &ang );
	if ( player == nullptr )
	{
		game_warning_( "Failed to spawn in player!\n" );
		return;
	}
}

void qm1_world_spawn_( ApeRoom *room )
{
	ApeWorldNode *roomNode = APE_WORLD_NODE( room );
	ape_world_node_attach( APE_WORLD_NODE( qm1_state_.camera ), roomNode );
	ape_world_node_set_position( APE_WORLD_NODE( qm1_state_.camera ), &QM_MATH_VECTOR3F( 0.0f, 128.0f, 0.0f ) );

	game_integrations_discord_update_activity_( "Preparing", nullptr, "qm1-logo", NIH_GAME_TITLE );

	spawn_player();

	qm1_state_.roundStatus = QM1_ROUND_STATUS_INTRO;
}
