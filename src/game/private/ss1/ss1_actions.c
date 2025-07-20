// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: This handles the actions specific to SS1/QM1.
// Author:  Mark E. Sowden

#include "ss1_game.h"

static void fire_decal( ApeInputState state, const char * )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
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

	//TODO: sigh...
	dir = PlInverseVector3( dir );

	game_print_( "Firing decal!\n" );

	if ( !game_test_fire_decal_( room, &pos, &dir ) )
	{
		game_print_( "Miss :(\n" );
	}
	else
	{
		game_print_( "Hit!\n" );
	}
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
