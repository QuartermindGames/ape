// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for Detox game project.

#include "tox_game.h"
#include "tox_character.h"
#include "tox_world.h"

ToxGlobalVars tox_globalVars;

static ApeCamera *playerCamera = NULL;

ApeCamera *tox_get_player_camera( void )
{
	return playerCamera;
}

static void move_camera_callback( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 pos = arl_camera_get_position( playerCamera );
	PLVector3 ang = arl_camera_get_angles( playerCamera );
	if ( strcmp( id, "rotateLeft" ) == 0 )
		ang.y += 1.5f;
	else if ( strcmp( id, "rotateRight" ) == 0 )
		ang.y -= 1.5f;

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	if ( strcmp( id, "moveForward" ) == 0 )
		pos = PlAddVector3( pos, PlScaleVector3F( forward, 0.5f ) );
	else if ( strcmp( id, "moveBackward" ) == 0 )
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, 0.5f ) );
	else if ( strcmp( id, "moveLeft" ) == 0 )
		pos = PlAddVector3( pos, PlScaleVector3F( left, 0.5f ) );
	else if ( strcmp( id, "moveRight" ) == 0 )
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, 0.5f ) );
	else if ( strcmp( id, "moveUp" ) == 0 )
		pos.y += 0.5f;
	else if ( strcmp( id, "moveDown" ) == 0 )
		pos.y -= 0.5f;

	arl_camera_set_position( playerCamera, &pos );
	arl_camera_set_angles( playerCamera, &ang );
}

static void spawn_light_action( ApeInputState state, PL_UNUSED const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	ApeWorld *world = acl_world_get_current();
	if ( world == NULL )
		return;

	static unsigned int delay = 0;
	if ( apeGetNumTicks() <= delay )
		return;

	PLVector3 pos = arl_camera_get_position( playerCamera );
	ape_world_attach_light( world,
	                        ape_light_create(
	                                &pos,
	                                &( PLColourF32 ){ 1.0f, 1.0f, 1.0f, 1.0f },
	                                2.5f,
	                                APE_LIGHT_TYPE_OMNI,
	                                APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC ) );

	delay = apeGetNumTicks() + 100;
}

static void progress_time_action( ApeInputState state, PL_UNUSED const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	ToxWorldState *worldState = tox_world_get_state();
	if ( worldState == NULL )
		return;

	if ( strcmp( id, "time_forward" ) == 0 )
		worldState->seconds += TOX_WORLD_SECONDS_TO_HOUR / 100;
	else
		worldState->seconds -= TOX_WORLD_SECONDS_TO_HOUR / 100;
}

static void print_pos_command( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	PLVector3 cameraPos = arl_camera_get_position( playerCamera );
	Game_Print( "Camera Pos: %s\n", PlPrintVector3( &cameraPos, PL_VAR_F32 ) );
	PLVector3 cameraAngles = arl_camera_get_angles( playerCamera );
	Game_Print( "Camera Ang: %s\n", PlPrintVector3( &cameraAngles, PL_VAR_F32 ) );
}

static void set_time_command( unsigned int argc, char **argv )
{
	ToxWorldState *worldState = tox_world_get_state();
	worldState->seconds = strtoul( argv[ 1 ], NULL, 10 );
}

static void initialize_game( void )
{
	PlRegisterConsoleVariable( "sun_yaw", "Set the yaw of the sun.", "0", PL_VAR_F32, &tox_globalVars.sunYaw, NULL, false );

	PlRegisterConsoleCommand( "print_pos", "Print the camera position.", 0, print_pos_command );
	PlRegisterConsoleCommand( "set_time", "Sets the world time.", 1, set_time_command );

	game_register_standard_entity_components();

	acl_entity_register_class( tox_character_get_class_table() );

	playerCamera = arl_camera_create( "test", &PLVector3( -2.37f, 1.0f, 1.70f ), &PLVector3( 0.0f, -165.0f, 0.0f ) );
	arl_camera_make_active( playerCamera );

	// hack hack hack hack hack hack
	PlParseConsoleString( "level ship/worlds/alive_intro.wld.n" );
	PlParseConsoleString( "r/fov 75" );
	PlParseConsoleString( "world/showallrooms true" );
	//PlParseConsoleString( "world/sortlights false" );

	// movement actions
	acl_input_register_action( "moveForward", ( ApeInputButton[] ){ APE_INPUT_UP }, 1, ( ApeInputKey[] ){ KEY_UP, 'w' }, 2, move_camera_callback );
	acl_input_register_action( "moveBackward", ( ApeInputButton[] ){ APE_INPUT_DOWN }, 1, ( ApeInputKey[] ){ KEY_DOWN, 's' }, 2, move_camera_callback );
	acl_input_register_action( "moveLeft", ( ApeInputButton[] ){ INPUT_LEFT }, 1, ( ApeInputKey[] ){ 'a' }, 1, move_camera_callback );
	acl_input_register_action( "moveRight", ( ApeInputButton[] ){ INPUT_RIGHT }, 1, ( ApeInputKey[] ){ 'd' }, 1, move_camera_callback );
	acl_input_register_action( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_callback );
	acl_input_register_action( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_callback );
	acl_input_register_action( "rotateLeft", NULL, 0, ( ApeInputKey[] ){ KEY_LEFT }, 1, move_camera_callback );
	acl_input_register_action( "rotateRight", NULL, 0, ( ApeInputKey[] ){ KEY_RIGHT }, 1, move_camera_callback );

	// this remaining bunch are for debugging purposes...
	acl_input_register_action( "spawn_light", NULL, 0, ( ApeInputKey[] ){ KEY_ENTER }, 1, spawn_light_action );
	acl_input_register_action( "time_forward", NULL, 0, ( ApeInputKey[] ){ 'z' }, 1, progress_time_action );
	acl_input_register_action( "time_backward", NULL, 0, ( ApeInputKey[] ){ 'x' }, 1, progress_time_action );
}

static void shutdown_game( void )
{
	arl_camera_destroy( playerCamera );
	playerCamera = NULL;
}

static void tick_game( void )
{
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
	{
		int mx, my;
		apeGetMouseDelta( &mx, &my );

		PLVector3 ang = arl_camera_get_angles( playerCamera );
		ang.y += ( float ) mx;
		ang.x += ( float ) my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
		arl_camera_set_angles( playerCamera, &ang );
	}

	tox_world_tick();
}

static bool handle_request( GameModeRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		case GAMEMODE_REQUEST_INITIALIZE:
		{
			initialize_game();
			return true;
		}
		case GAMEMODE_REQUEST_TICK:
		{
			tick_game();
			break;
		}
		case GAMEMODE_REQUEST_HANDLEINPUT:
		{
			break;
		}
		case GAMEMODE_REQUEST_SPAWNWORLD:
		{
			tox_world_spawn( ( ApeWorld * ) user );
			break;
		}
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameGetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );
	gameMode.requestCallbackMethod = handle_request;
	return &gameMode;
}
