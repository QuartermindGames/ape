// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: This handles the actions specific to SS1/QM1.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "ss1_game.h"

#include "entities/qm1/qm1_entity_player.h"

static void fire_decal( ApeInputState state, const char * )
{
	static int64_t nextFire = 0;
	if ( nextFire > ape_get_num_ticks() )
	{
		return;
	}

	if ( state & APE_INPUT_STATE_RELEASED )
	{
		return;
	}

	if ( ss1_gameState.camera == nullptr )
	{
		return;
	}

	ApeRoom *room = ape_camera_get_room( ss1_gameState.camera );
	if ( room == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( ss1_gameState.camera );
	QmMathVector3f dir = ape_camera_get_forward( ss1_gameState.camera );
	dir                = qm_math_vector3f_invert( dir );//TODO: sigh... camera is inverted

	// apply some randomisation to the fire direction

	unsigned int seed = qm_os_random_seed_initialize();

	float          spreadAmount = PL_DEG2RAD( 16.0f );
	QmMathVector3f spread       = qm_math_vector3f(
            qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount,
            qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount,
            qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount );

	dir = qm_math_vector3f_add( dir, spread );
	dir = qm_math_vector3f_normalize( dir );

	game_test_fire_decal_( room, &pos, &dir );

	nextFire = ape_get_num_ticks() + 8;
}

static void toggle_camera( ApeInputState state, PL_UNUSED const char *id )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	ss1_gameState.oldCameraState = ss1_gameState.cameraState;

	ss1_gameState.cameraState++;
	if ( ss1_gameState.cameraState >= GAME_CAMERA_STATE_MAX )
	{
		ss1_gameState.cameraState = 0;
	}

	switch ( ss1_gameState.cameraState )
	{
		default:
		case GAME_CAMERA_STATE_FREE:
			game_print_( "Free Camera\n" );
			break;
		case GAME_CAMERA_STATE_FIRST_PERSON:
			game_print_( "First-Person Camera\n" );
			break;
		case GAME_CAMERA_STATE_THIRD_PERSON:
			game_print_( "Third-Person Camera\n" );
			break;
	}

	// attempt to hide the player model

	ApeEntity *entity = game_server_get_host_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( entity );
	if ( playerEntity == nullptr || playerEntity->model == nullptr )
	{
		return;
	}

	ApeWorldNode *worldNode = APE_WORLD_NODE( playerEntity->model );
	if ( ss1_gameState.cameraState == GAME_CAMERA_STATE_FIRST_PERSON )
	{
		worldNode->flags |= APE_WORLD_NODE_FLAG_HIDDEN;
	}
	else
	{
		worldNode->flags &= ~APE_WORLD_NODE_FLAG_HIDDEN;
	}
}

void ss1_actions_register_()
{
	ape_client_input_register_action( "ss1_toggle_camera", ( ApeInputButton[] ) { INPUT_BACK }, 1, ( ApeInputKey[] ) { 'z' }, 1, toggle_camera );
	ape_client_input_register_action( "ss1_fire_decal", ( ApeInputButton[] ) { INPUT_Y }, 1, ( ApeInputKey[] ) { 'v' }, 1, fire_decal );
}
