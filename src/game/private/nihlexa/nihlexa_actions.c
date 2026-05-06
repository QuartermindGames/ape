// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: This handles the actions specific to SS1/QM1.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "nihlexa.h"
#include "components/component_camera.h"

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

	if ( qm1_state_.camera == nullptr )
	{
		return;
	}

	ApeRoom *room = ape_camera_get_room( qm1_state_.camera );
	if ( room == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( qm1_state_.camera );
	QmMathVector3f dir = ape_camera_get_forward( qm1_state_.camera );
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

	ApeEntity *entity = game_server_get_host_entity_();
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
	if ( qm1_state_.cameraState == GAME_CAMERA_STATE_FIRST_PERSON )
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

	if ( qm1_state_.camera == nullptr )
	{
		return;
	}

	ApeRoom *room = ape_camera_get_room( qm1_state_.camera );
	if ( room == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( qm1_state_.camera );
	QmMathVector3f dir = ape_camera_get_forward( qm1_state_.camera );
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

static void camera_input( ApeInputState state, const char *id )
{
	if ( state & APE_INPUT_STATE_RELEASED )
	{
		return;
	}

	ApeCamera *camera = qm1_state_.camera;
	if ( camera == nullptr )
	{
		return;
	}

	QmMathVector3f angles = ape_camera_get_angles( camera );

	if ( strcmp( "qm1_camera_rotate_left", id ) == 0 )
	{
		angles.y += 0.5f;
	}
	else if ( strcmp( "qm1_camera_rotate_right", id ) == 0 )
	{
		angles.y -= 0.5f;
	}
	else if ( strcmp( "qm1_camera_rotate_up", id ) == 0 )
	{
		angles.x += 0.5f;
	}
	else if ( strcmp( "qm1_camera_rotate_down", id ) == 0 )
	{
		angles.x -= 0.5f;
	}

	ape_camera_set_angles( camera, &angles );
}

void ss1_actions_register_()
{
	ape_client_input_register_action( "qm1_toggle_camera", ( ApeInputButton[] ) { INPUT_BACK }, 1, ( ApeInputKey[] ) { 'z' }, 1, toggle_camera );
	ape_client_input_register_action( "qm1_fire_decal", ( ApeInputButton[] ) { INPUT_Y }, 1, ( ApeInputKey[] ) { 'v' }, 1, fire_decal );
	ape_client_input_register_action( "qm1_spawn_portal", ( ApeInputButton[] ) { INPUT_X }, 1, ( ApeInputKey[] ) { 'x' }, 1, spawn_portal_action );

	ape_client_input_register_action( "qm1_camera_rotate_left", nullptr, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_LEFT }, 1, camera_input );
	ape_client_input_register_action( "qm1_camera_rotate_right", nullptr, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_RIGHT }, 1, camera_input );
	ape_client_input_register_action( "qm1_camera_rotate_up", nullptr, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_UP }, 1, camera_input );
	ape_client_input_register_action( "qm1_camera_rotate_down", nullptr, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_DOWN }, 1, camera_input );
}
