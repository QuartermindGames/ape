// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Client input actions.
// Author:  Mark E. Sowden

#include "game_private.h"
#include "game_server.h"
#include "components/component_camera.h"

#include "components/component_movement.h"

#include "menu/menu.h"

static void say_action( ApeInputState state, const char *id )
{
	//TODO: open chat prompt
}

static void move_action( const ApeInputState state, const char *id )
{
	if ( game_menu_is_open() )
	{
		return;
	}

	if ( !( state & APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	ApeEntity *entity = game_server_get_local_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	GameMovementComponent *movementComponent = ape_entity_get_component( entity, "movement" );
	if ( movementComponent == nullptr )
	{
		game_warning_( "Player is possessing an entity without a movement component!\n" );
		return;
	}

	QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( entity ) );
	if ( strcmp( id, "game_turn_left" ) == 0 )
	{
		ang.y += 1.0f;
	}
	else if ( strcmp( id, "game_turn_right" ) == 0 )
	{
		ang.y -= 1.0f;
	}

	ape_world_node_set_angles( APE_WORLD_NODE( entity ), &ang );

	QmMathVector3f forward, left;
	PlAnglesAxes( ang, &left, nullptr, &forward );

	forward.y = 0.0f;
	left.y    = 0.0f;

	forward = qm_math_vector3f_normalize( forward );
	left    = qm_math_vector3f_normalize( left );

	movementComponent->direction = ( QmMathVector3f ) {};
	if ( strcmp( id, "game_move_forward" ) == 0 )
	{
		movementComponent->direction = qm_math_vector3f_scale_float( forward, 1.0f );
	}
	else if ( strcmp( id, "game_move_backward" ) == 0 )
	{
		movementComponent->direction = qm_math_vector3f_scale_float( forward, -1.0f );
	}
	if ( strcmp( id, "game_strafe_left" ) == 0 )
	{
		movementComponent->direction = qm_math_vector3f_scale_float( left, 1.0f );
	}
	else if ( strcmp( id, "game_strafe_right" ) == 0 )
	{
		movementComponent->direction = qm_math_vector3f_scale_float( left, -1.0f );
	}

	if ( strcmp( id, "game_jump" ) == 0 && movementComponent->isGrounded )
	{
		movementComponent->direction.y = 1.0f;
	}
}

void game_client_actions_register_()
{
	ape_client_input_register_action( "game_say", nullptr, 0, ( ApeInputKey[] ) { 't' }, 1, say_action );

	// player movement

	ape_client_input_register_action( "game_move_forward", nullptr, 0, ( ApeInputKey[] ) { 'w', APE_INPUT_KEY_UP }, 2, move_action );
	ape_client_input_register_action( "game_move_backward", nullptr, 0, ( ApeInputKey[] ) { 's', APE_INPUT_KEY_DOWN }, 2, move_action );
	ape_client_input_register_action( "game_strafe_left", nullptr, 0, ( ApeInputKey[] ) { 'a' }, 1, move_action );
	ape_client_input_register_action( "game_strafe_right", nullptr, 0, ( ApeInputKey[] ) { 'd' }, 1, move_action );
	ape_client_input_register_action( "game_turn_left", nullptr, 0, ( ApeInputKey[] ) { APE_INPUT_KEY_LEFT }, 1, move_action );
	ape_client_input_register_action( "game_turn_right", nullptr, 0, ( ApeInputKey[] ) { APE_INPUT_KEY_RIGHT }, 1, move_action );

	ape_client_input_register_action( "game_jump", ( ApeInputButton[] ) { INPUT_A }, 1, ( ApeInputKey[] ) { ' ' }, 1, move_action );
}
