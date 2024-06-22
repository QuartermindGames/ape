// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Main file for Detox game project.

#include "tox_game.h"
#include "tox_world.h"

#include "ui/tox_ui.h"

ToxGlobalVars tox_globalVars;

static ApeCamera *playerCamera = NULL;

ApeCamera *tox_get_player_camera( void )
{
	return playerCamera;
}

static void move_camera_iso_callback( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 ang = ape_camera_get_angles( playerCamera );

	ang.x = 0.0f;
	if ( strcmp( id, "rotateLeft" ) == 0 )
	{
		ang.y += 1.5f;
	}
	else if ( strcmp( id, "rotateRight" ) == 0 )
	{
		ang.y -= 1.5f;
	}

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	static const float SPEED = 0.5f;

	PLVector3 pos = ape_camera_get_position( playerCamera );
	if ( strcmp( id, "moveForward" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( forward, SPEED ) );
	}
	else if ( strcmp( id, "moveBackward" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, SPEED ) );
	}
	else if ( strcmp( id, "moveLeft" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( left, SPEED ) );
	}
	else if ( strcmp( id, "moveRight" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, SPEED ) );
	}
	else if ( strcmp( id, "moveUp" ) == 0 )
	{
		pos.y += 0.5f;
	}
	else if ( strcmp( id, "moveDown" ) == 0 )
	{
		pos.y -= 0.5f;
	}

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static void move_camera_callback( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	PLVector3 pos = ape_camera_get_position( playerCamera );
	PLVector3 ang = ape_camera_get_angles( playerCamera );
	if ( strcmp( id, "rotateLeft" ) == 0 )
	{
		ang.y += 1.5f;
	}
	else if ( strcmp( id, "rotateRight" ) == 0 )
	{
		ang.y -= 1.5f;
	}

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	static const float SPEED = 2.0f;

	if ( strcmp( id, "moveForward" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( forward, SPEED ) );
	}
	else if ( strcmp( id, "moveBackward" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, SPEED ) );
	}
	else if ( strcmp( id, "moveLeft" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( left, SPEED ) );
	}
	else if ( strcmp( id, "moveRight" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, SPEED ) );
	}
	else if ( strcmp( id, "moveUp" ) == 0 )
	{
		pos.y += 0.5f;
	}
	else if ( strcmp( id, "moveDown" ) == 0 )
	{
		pos.y -= 0.5f;
	}

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static void rotate_camera_action( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 ang = ape_camera_get_angles( playerCamera );
	if ( strcmp( id, "rotateUp" ) == 0 )
	{
		ang.x += 1.5f;
	}
	else if ( strcmp( id, "rotateDown" ) == 0 )
	{
		ang.x -= 1.5f;
	}

	ang.x = PlClamp( -90.0f, ang.x, 90.0f );

	ape_camera_set_angles( playerCamera, &ang );
}

static void progress_time_action( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
	{
		return;
	}

	ToxWorldState *worldState = tox_world_get_state();
	if ( worldState == NULL )
	{
		return;
	}

	if ( strcmp( id, "tox_time_forward" ) == 0 )
	{
		worldState->seconds += TOX_WORLD_SECONDS_TO_HOUR / 10;
	}
	else
	{
		worldState->seconds -= TOX_WORLD_SECONDS_TO_HOUR / 10;
	}
}

static void print_pos_command( unsigned int, char ** )
{
	PLVector3 cameraPos = ape_camera_get_position( playerCamera );
	game_print_( "Camera Pos: %s\n", PlPrintVector3( &cameraPos, PL_VAR_F32 ) );
	PLVector3 cameraAngles = ape_camera_get_angles( playerCamera );
	game_print_( "Camera Ang: %s\n", PlPrintVector3( &cameraAngles, PL_VAR_F32 ) );
}

static void set_time_command( unsigned int, char **argv )
{
	ToxWorldState *worldState = tox_world_get_state();
	worldState->seconds = strtoul( argv[ 1 ], NULL, 10 );
}

static void damage_player_command( unsigned int argc, char **argv )
{
	int16_t value;
	if ( argc > 1 )
	{
		value = ( int16_t ) atoi( argv[ 1 ] );
	}
	else
	{
		value = 10;
	}

	tox_ui_handle_damage_event( value );
}

static void damage_player_action( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
	{
		return;
	}

	tox_ui_handle_damage_event( 10 );
}

extern const ApeEntityClassDefinition *tox_characterClass;

static bool initialize_game( void )
{
	PlRegisterConsoleVariable( "tox_time_speed", "Sets the speed of time.", "2000", PL_VAR_F32, &tox_globalVars.timeSpeed, NULL, false );

	PlRegisterConsoleCommand( "tox_print_pos", "Print the camera position and angles.", 0, print_pos_command );
	PlRegisterConsoleCommand( "tox_set_time", "Sets the world time.", 1, set_time_command );
	PlRegisterConsoleCommand( "tox_damage_player", "Damage the player by a specific amount.", -1, damage_player_command );

	// movement actions
	ape_client_input_register_action( "moveForward", ( ApeInputButton[] ){ APE_INPUT_UP }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_UP, 'w' }, 2, move_camera_callback );
	ape_client_input_register_action( "moveBackward", ( ApeInputButton[] ){ APE_INPUT_DOWN }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_DOWN, 's' }, 2, move_camera_callback );
	ape_client_input_register_action( "moveLeft", ( ApeInputButton[] ){ INPUT_LEFT }, 1, ( ApeInputKey[] ){ 'a' }, 1, move_camera_callback );
	ape_client_input_register_action( "moveRight", ( ApeInputButton[] ){ INPUT_RIGHT }, 1, ( ApeInputKey[] ){ 'd' }, 1, move_camera_callback );
	ape_client_input_register_action( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_callback );
	ape_client_input_register_action( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_callback );
	ape_client_input_register_action( "rotateLeft", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_LEFT }, 1, move_camera_callback );
	ape_client_input_register_action( "rotateRight", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_RIGHT }, 1, move_camera_callback );

	// this remaining bunch are for debugging purposes...
	ape_client_input_register_action( "tox_time_forward", NULL, 0, ( ApeInputKey[] ){ 'z' }, 1, progress_time_action );
	ape_client_input_register_action( "tox_time_backward", NULL, 0, ( ApeInputKey[] ){ 'x' }, 1, progress_time_action );
	ape_client_input_register_action( "tox_damage_player", NULL, 0, ( ApeInputKey[] ){ 'c' }, 1, damage_player_action );

	ape_client_input_register_action( "rotateUp", NULL, 0, ( ApeInputKey[] ){ 'r' }, 1, rotate_camera_action );
	ape_client_input_register_action( "rotateDown", NULL, 0, ( ApeInputKey[] ){ 'f' }, 1, rotate_camera_action );

	tox_ui_initialize();

	ss_game_register_standard_entity_components_();

	ape_register_entity_class( tox_characterClass );

	playerCamera = ape_create_camera( nullptr, &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
	if ( playerCamera == NULL )
	{
		game_error_( "Failed to create player camera!\n" );
		return false;
	}

	return true;
}

static bool shutdown_game( void )
{
	tox_ui_shutdown();

	ape_world_node_destroy( ape_camera_get_world_node( playerCamera ) );
	playerCamera = nullptr;

	return true;
}

static void ss1_handle_input( void )
{
	PLVector3 ang = ape_camera_get_angles( playerCamera );
	PLVector3 pos = ape_camera_get_position( playerCamera );

	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
	{
		int mx, my;
		ape_client_input_get_mouse_delta( &mx, &my );


		ang.y += ( float ) mx;
		ang.x += ( float ) my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
	}

	PLVector2 rightStick = ape_client_input_get_controller_axis_state( 0, 1 );
	ang.x -= rightStick.y * 2.0f;
	ang.y -= rightStick.x * 2.0f;

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	PLVector2 leftStick = ape_client_input_get_controller_axis_state( 0, 0 );
	pos = PlSubtractVector3( pos, PlScaleVector3F( forward, leftStick.y ) );
	pos = PlSubtractVector3( pos, PlScaleVector3F( left, leftStick.x ) );

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static bool tick_game( void )
{
	ss1_handle_input();

	tox_world_tick( ss_game_get_current_world() );
	tox_ui_tick();

	return true;
}

static bool draw_game( ApeViewport *viewport )
{
	ape_camera_make_active( playerCamera );
	ape_camera_draw_perspective( playerCamera, viewport );
	return true;
}

static bool handle_request( ApeGameInterfaceRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
		{
			return initialize_game();
		}
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN:
		{
			return shutdown_game();
		}
		case APE_GAME_INTERFACE_REQUEST_TICK_SERVER:
		{
			return tick_game();
		}
		case APE_GAME_INTERFACE_REQUEST_DRAW:
		{
			return draw_game( ( ApeViewport * ) user );
		}
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
		{
			return tox_ui_draw( ( ApeViewport * ) user );
		}
		case APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT:
		{
			break;
		}
		case APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD:
		{
			tox_world_spawn( ( ApeWorld * ) user );
			return true;
		}
		default:
			break;
	}

	return false;
}

static void server_client_connected( ApeServerClientHandle *clientHandle )
{
	game_server_client_connected_( clientHandle );
}

static void server_client_disconnected( ApeServerClientHandle *clientHandle )
{
	game_server_client_disconnected_( clientHandle );
}

static void server_process_message( ApeServerClientHandle *clientHandle, const void *buf, size_t bufSize )
{
	game_server_process_message_( clientHandle, buf, bufSize );
}

static void client_process_message( const void *buf, size_t bufSize )
{
	game_client_process_message_( buf, bufSize );
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode = {
	        .version = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = TOX_GAME_PROTOCOL_VERSION + GAME_NET_PROTOCOL_VERSION,
	        .identifier = "ss2",

	        .requestCallbackMethod = handle_request,

	        .serverClientConnected = server_client_connected,
	        .serverClientDisconnected = server_client_disconnected,
	        .serverProcessMessage = server_process_message,

	        .clientProcessMessage = client_process_message,
	};
	return &gameMode;
}
