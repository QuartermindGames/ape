// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: SS3 game implementation.
// Author:  Mark E. Sowden

#include "ss3_game.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeCamera *playerCamera = NULL;

static void move_camera_action( ApeInputState state, const char *id )
{
	static const float MOVE_SPEED = 0.5f;
	static const float TURN_SPEED = 1.5f;

	if ( !( state & APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	PLVector3 pos = ape_camera_get_position( playerCamera );
	PLVector3 ang = ape_camera_get_angles( playerCamera );
	if ( strcmp( id, "ss3_turn_left" ) == 0 )
	{
		ang.y += TURN_SPEED;
	}
	else if ( strcmp( id, "ss3_turn_right" ) == 0 )
	{
		ang.y -= TURN_SPEED;
	}

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	if ( strcmp( id, "ss3_move_forward" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( forward, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_backward" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_left" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( left, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_right" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_up" ) == 0 )
	{
		pos.y += 0.5f;
	}
	else if ( strcmp( id, "ss3_move_down" ) == 0 )
	{
		pos.y -= 0.5f;
	}

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static bool initialize( void )
{
	ss_game_register_standard_entity_components_();

	playerCamera = ape_camera_create( "ss3_camera_player", &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE );
	if ( playerCamera == NULL )
	{
		Game_Error( "Failed to create player camera!\n" );
		return false;
	}

	// movement actions
	ape_client_input_register_action( "ss3_move_forward", ( ApeInputButton[] ){ APE_INPUT_UP }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_UP, 'w' }, 2, move_camera_action );
	ape_client_input_register_action( "ss3_move_backward", ( ApeInputButton[] ){ APE_INPUT_DOWN }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_DOWN, 's' }, 2, move_camera_action );
	ape_client_input_register_action( "ss3_move_left", ( ApeInputButton[] ){ INPUT_LEFT }, 1, ( ApeInputKey[] ){ 'a' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_move_right", ( ApeInputButton[] ){ INPUT_RIGHT }, 1, ( ApeInputKey[] ){ 'd' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_move_down", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_move_up", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_turn_left", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_LEFT }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_turn_right", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_RIGHT }, 1, move_camera_action );

	return true;
}

static bool handle_request( ApeGameInterfaceRequest request, void *user )
{
	switch ( request )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return initialize();
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN: break;
		case APE_GAME_INTERFACE_REQUEST_DRAW: break;
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI: break;
		case APE_GAME_INTERFACE_REQUEST_TICK: break;
		case APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT: break;
		case APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD: break;
		case APE_GAME_INTERFACE_REQUEST_DISCONNECT: break;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

static const ApeGameInterfaceImport apeImport = {
        .requestCallbackMethod = handle_request,
};

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	return &apeImport;
}
