// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for demo game project.

#include "mag_game.h"

static SSArlCamera *playerCamera = NULL;

static ApeMaterial *testMaterial;

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
	game_register_standard_entity_components();

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

	testMaterial = ss_arl_material_cache( "materials/debug/debug_sprite.mat.n", APE_CACHE_EDITOR, true, false );

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

static void draw_game_ui( SSArlViewport *viewport )
{
#if 0
	SSArlRenderTarget *renderTarget = ss_arl_viewport_get_render_target( gameViewport );
	if ( renderTarget != NULL )
	{
		PLGTexture *texture = ss_arl_render_target_get_texture( renderTarget );
		if ( texture != NULL )
		{
			float x = ( float ) viewport->x;
			float y = ( float ) viewport->y;
			float w = ( float ) viewport->width;
			float h = ( float ) viewport->height;

			PlgSetCullMode( PLG_CULL_NEGATIVE );

			PlgSetShaderProgram( ss_arl_shader_get_default( APE_SHADER_DEFAULT ) );
			PlgSetTexture( texture, 0 );

			PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

			PlgImmPushVertex( x, y + h, 0.0f );
			PlgImmColour( 255, 255, 255, 255 );
			PlgImmTextureCoord( 0.0f, 0.0f );

			PlgImmPushVertex( x, y, 0.0f );
			PlgImmColour( 255, 255, 255, 255 );
			PlgImmTextureCoord( 0.0f, 1.0f );

			PlgImmPushVertex( x + w, y + h, 0.0f );
			PlgImmColour( 255, 255, 255, 255 );
			PlgImmTextureCoord( 1.0f, 0.0f );

			PlgImmPushVertex( x + w, y, 0.0f );
			PlgImmColour( 255, 255, 255, 255 );
			PlgImmTextureCoord( 1.0f, 1.0f );

			PlgImmDraw();

			PlgSetCullMode( PLG_CULL_POSITIVE );
		}
	}
#endif

	SS_Arl_BitmapFont *font = ss_arl_get_default_bitmap_font();
	const char *mode;
	if ( mag_tile_editor_get_status() == MAG_TILE_EDITOR_STATUS_OPEN )
		mode = "Tile Editor";
	else
		mode = "Game";

	char buf[ 64 ];
	snprintf( buf, sizeof( buf ), "MODE = %s", mode );
	ss_arl_bitmap_font_draw_string( font, 10.0f, 50.0f, 1.0f, 1.0f, PL_COLOUR_GREEN, buf, false );

#if 1
	static float rotate = 0.0f;
	ss_arl_draw_sprite( testMaterial,
	                    &( PLQuad ){ 0.0f, 0.0f, 128.0f, 128.0f },
	                    &( PLVector3 ){ 250.f, 250.f, 0.f },
	                    &( PLVector3 ){ -( 128.0f / 2.0f ), -( 128.0f / 2.0f ), 0.0f },
	                    &( PLVector3 ){ 0.0f, 0.0f, rotate }, 1.0f );
	rotate += 0.0005f;
#endif

	mag_tile_editor_draw( viewport );
}

static bool handle_request( GameModeRequest modeRequest, void *user )
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
		case GAME_MODE_REQUEST_DRAW_UI:
			draw_game_ui( ( SSArlViewport * ) user );
			return true;
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *gameGetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );
	gameMode.requestCallbackMethod = handle_request;
	return &gameMode;
}
