// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for demo game project.

#include "mag_game.h"
#include "mag_world.h"

static SSArlCamera *playerCamera = NULL;

static ApeWorld *world;

static void move_camera_callback( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 pos = ss_arl_camera_get_position( playerCamera );
	PLVector3 ang = ss_arl_camera_get_angles( playerCamera );
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

	ss_arl_camera_set_position( playerCamera, &pos );
	ss_arl_camera_set_angles( playerCamera, &ang );
}

static bool initialize_game( void )
{
	ss_game_register_standard_entity_components_();

	playerCamera = ss_arl_camera_create( "mag_camera", &pl_vecOrigin3, &pl_vecOrigin3, SS_ARL_CAMERA_MODE_PERSPECTIVE );
	if ( playerCamera == NULL )
	{
		Game_Error( "Failed to create game camera!\n" );
		return false;
	}

	ss_acl_input_register_action( "moveForward", NULL, 0, ( ApeInputKey[] ){ KEY_UP, 'w' }, 2, move_camera_callback );
	ss_acl_input_register_action( "moveBackward", NULL, 0, ( ApeInputKey[] ){ KEY_DOWN, 's' }, 2, move_camera_callback );
	ss_acl_input_register_action( "moveLeft", NULL, 0, ( ApeInputKey[] ){ 'a' }, 1, move_camera_callback );
	ss_acl_input_register_action( "moveRight", NULL, 0, ( ApeInputKey[] ){ 'd' }, 1, move_camera_callback );
	ss_acl_input_register_action( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_callback );
	ss_acl_input_register_action( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_callback );

	world = ss_acl_level_create();
	//ss_game_spawn_world( world );

	ss_arl_camera_assign_world( playerCamera, world );

	mag_tile_editor_initialize();

	return true;
}

static void shutdown_game( void )
{
	ss_arl_camera_destroy( playerCamera );
	playerCamera = NULL;
}

static void tick_game( void )
{
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
	{
		int mx, my;
		ss_acl_input_get_mouse_delta( &mx, &my );

		PLVector3 ang = ss_arl_camera_get_angles( playerCamera );
		ang.y += mx;
		ang.x += my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
		ss_arl_camera_set_angles( playerCamera, &ang );
	}
}

static void draw_game( SSArlViewport * )
{
}

static void draw_game_ui( SSArlViewport *viewport )
{
	SSArlCamera *oldCamera = ss_arl_viewport_get_camera( viewport );
	//ss_arl_camera_make_active( playerCamera );

	PLGCamera *internalCamera = ss_arl_camera_get_internal( playerCamera );
	assert( internalCamera != NULL );
	if ( internalCamera == NULL )
		return;

	mag_world_draw( viewport );

	if ( oldCamera != NULL )
		ss_arl_camera_make_active( oldCamera );

	SS_Arl_BitmapFont *font = ss_arl_get_default_bitmap_font();
	const char *mode;
	if ( mag_tile_editor_get_status() == MAG_TILE_EDITOR_STATUS_OPEN )
		mode = "Tile Editor";
	else
		mode = "Game";

	char buf[ 64 ];
	snprintf( buf, sizeof( buf ), "MODE = %s", mode );
	ss_arl_bitmap_font_draw_string( font, 10.0f, 50.0f, 1.0f, 1.0f, PL_COLOUR_GREEN, buf, false );

	mag_tile_editor_draw( viewport );
}

static bool handle_request( SSGameModeRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		case GAMEMODE_REQUEST_INITIALIZE:
			return initialize_game();
		case GAMEMODE_REQUEST_SHUTDOWN:
			shutdown_game();
			return true;
		case GAMEMODE_REQUEST_TICK:
			tick_game();
			return true;
		case GAME_MODE_REQUEST_DRAW:
			draw_game( ( SSArlViewport * ) user );
			return true;
		case GAME_MODE_REQUEST_DRAW_UI:
			draw_game_ui( ( SSArlViewport * ) user );
			return true;
		default:
			break;
	}

	return false;
}

const SSGameModeInterface *gameModeInterface;
const SSGameModeInterface *ss_game_mode_get_interface( void )
{
	static SSGameModeInterface gameMode;
	PL_ZERO_( gameMode );
	gameMode.requestCallbackMethod = handle_request;
	return &gameMode;
}
