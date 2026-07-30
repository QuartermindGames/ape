// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: This handles the actions specific to SS1/QM1.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "nihlexa.h"
#include "components/component_camera.h"

#include "entities/qm1/qm1_entity_player.h"

static void fire_decal( ApeInputState state, const char * )
{
	GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr || player->camera == nullptr )
	{
		return;
	}

	static int64_t nextFire = 0;
	if ( nextFire > ape_get_num_ticks() )
	{
		return;
	}

	if ( state & APE_INPUT_STATE_RELEASED )
	{
		return;
	}

	ApeRoom *room = ape_camera_get_room( player->camera );
	if ( room == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( player->camera );
	QmMathVector3f dir = ape_camera_get_forward( player->camera );
	dir                = qm_math_vector3f_invert( dir );//TODO: sigh... camera is inverted

	// apply some randomisation to the fire direction

	unsigned int seed = qm_os_random_seed_initialize();

	float          spreadAmount = QM_MATH_DEG2RAD( 16.0f );
	QmMathVector3f spread       = qm_math_vector3f(
            qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount,
            qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount,
            qm_os_random_uniform_float( &seed, spreadAmount ) * spreadAmount );

	dir = qm_math_vector3f_add( dir, spread );
	dir = qm_math_vector3f_normalize( dir );

	game_test_fire_decal_( room, &pos, &dir );

	nextFire = ape_get_num_ticks() + 8;
}

static void toggle_camera( ApeInputState state, [[maybe_unused]] const char *id )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	ApeEntity *entity = game_server_get_local_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	GameCameraComponent *component = ape_entity_get_component( entity, GAME_CAMERA_COMPONENT_NAME );
	if ( component == nullptr )
	{
		return;
	}

	game_component_camera_cycle_state_( component );

#if 0
	// attempt to hide the player model

	Qm1PlayerEntity *playerEntity = QM1_PLAYER_ENTITY( entity );
	if ( playerEntity == nullptr || playerEntity->model == nullptr )
	{
		return;
	}

	ApeWorldNode *worldNode = APE_WORLD_NODE( playerEntity->model );
	if ( nih_serverState_.cameraState == GAME_CAMERA_STATE_FIRST_PERSON )
	{
		worldNode->flags |= APE_WORLD_NODE_FLAG_HIDDEN;
	}
	else
	{
		worldNode->flags &= ~APE_WORLD_NODE_FLAG_HIDDEN;
	}
#endif
}

static void spawn_portal_action( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr || player->camera == nullptr )
	{
		return;
	}

	ApeRoom *room = ape_camera_get_room( player->camera );
	if ( room == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( player->camera );
	QmMathVector3f dir = ape_camera_get_forward( player->camera );
	dir                = qm_math_vector3f_invert( dir );//TODO: sigh... camera is inverted

	ApeEntity *entity = ape_entity_create( APE_WORLD_NODE( room ), "portal", nullptr, nullptr, &pos, &dir );
	if ( entity != nullptr )
	{
		ape_entity_spawn( entity );

		char tmp[ 64 ];
		qm_math_vector3f_print( pos, tmp, sizeof( tmp ) );
		game_debug_( "Spawned portal entity at %s\n", tmp );
	}
}

#if 0
static void camera_input( ApeInputState state, const char *id )
{
	if ( state & APE_INPUT_STATE_RELEASED )
	{
		return;
	}

	GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr || player->camera == nullptr )
	{
		return;
	}

	ApeEntity *entity = game_server_get_local_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	GameCameraComponent *component = ape_entity_get_component( entity, GAME_CAMERA_COMPONENT_NAME );
	if ( component == nullptr || component->state != GAME_CAMERA_STATE_FREE )
	{
		return;
	}

	QmMathVector3f ang = ape_camera_get_angles( player->camera );
	QmMathVector3f pos = ape_camera_get_position( player->camera );

	float forwardMove = 0.0f;
	float sideMove    = 0.0f;

	if ( strcmp( "nih_camera_rotate_left", id ) == 0 )
	{
		ang.y += 0.5f;
	}
	else if ( strcmp( "nih_camera_rotate_right", id ) == 0 )
	{
		ang.y -= 0.5f;
	}
	else if ( strcmp( "nih_camera_rotate_up", id ) == 0 )
	{
		ang.x += 0.5f;
	}
	else if ( strcmp( "nih_camera_rotate_down", id ) == 0 )
	{
		ang.x -= 0.5f;
	}
	else if ( strcmp( "nih_camera_move_forward", id ) == 0 )
	{
		forwardMove -= 0.5f;
	}
	else if ( strcmp( "nih_camera_move_back", id ) == 0 )
	{
		forwardMove += 0.5f;
	}
	else if ( strcmp( "nih_camera_move_left", id ) == 0 )
	{
		sideMove -= 0.5f;
	}
	else if ( strcmp( "nih_camera_move_right", id ) == 0 )
	{
		sideMove += 0.5f;
	}

	QmMathVector3f forward, left;
	PlAnglesAxes( ang, &left, nullptr, &forward );
	pos = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( forward, forwardMove ) );
	pos = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( left, sideMove ) );

	ape_camera_set_angles( player->camera, &ang );
	ape_camera_set_position( player->camera, &pos );
}
#endif

void nih_actions_register_()
{
	ape_client_input_register_action( "nih_toggle_camera", ( ApeInputButton[] ) { INPUT_BACK }, 1, ( ApeInputKey[] ) { KEY_TAB }, 1, toggle_camera, 0 );
	ape_client_input_register_action( "nih_fire_decal", ( ApeInputButton[] ) { INPUT_Y }, 1, ( ApeInputKey[] ) { 'v' }, 1, fire_decal, 0 );
	ape_client_input_register_action( "nih_spawn_portal", ( ApeInputButton[] ) { INPUT_X }, 1, ( ApeInputKey[] ) { 'x' }, 1, spawn_portal_action, 0 );
}
