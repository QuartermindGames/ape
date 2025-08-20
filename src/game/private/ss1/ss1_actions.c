// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: This handles the actions specific to SS1/QM1.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "ss1_game.h"

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

	PLVector3 pos = ape_camera_get_position( ss1_gameState.camera );
	PLVector3 dir = ape_camera_get_forward( ss1_gameState.camera );
	dir           = PlInverseVector3( dir );//TODO: sigh... camera is inverted

	// apply some randomisation to the fire direction

	unsigned int seed = qm_os_random_seed_initialize();

	float     spreadAmount = PL_DEG2RAD( 16.0f );
	PLVector3 spread       = PL_VECTOR3(
            ( qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount ),
            ( qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount ),
            ( qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount ) );

	dir = PlAddVector3( dir, spread );
	dir = PlNormalizeVector3( dir );

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
	ss1_gameState.cameraState    = ss1_gameState.cameraState == SS1_CAMERA_STATE_THIRD_PERSON ? SS1_CAMERA_STATE_FREE : SS1_CAMERA_STATE_THIRD_PERSON;
}

void ss1_actions_register_()
{
	ape_client_input_register_action( "ss1_toggle_camera", ( ApeInputButton[] ) { INPUT_BACK }, 1, ( ApeInputKey[] ) { 'z' }, 1, toggle_camera );
	ape_client_input_register_action( "ss1_fire_decal", ( ApeInputButton[] ) { INPUT_Y }, 1, ( ApeInputKey[] ) { 'v' }, 1, fire_decal );
}
