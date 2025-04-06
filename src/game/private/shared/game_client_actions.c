// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"

#include "game_menu.h"
#include "game_server.h"

#include "components/component_movement.h"

static void say_action( ApeInputState state, const char *id )
{
	//TODO: open chat prompt
}

static void move_action( ApeInputState state, const char *id )
{
	if ( game_menu_is_open() )
	{
		return;
	}

	if ( !( state & APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	ApeEntity *entity = game_server_get_host_entity_();
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

	movementComponent->direction = ( PLVector3 ) {};
	if ( strcmp( id, "game_move_forward" ) == 0 )
	{
		movementComponent->direction.z = 1.0f;
	}
	else if ( strcmp( id, "game_move_backward" ) == 0 )
	{
		movementComponent->direction.z = -1.0f;
	}
	if ( strcmp( id, "game_strafe_left" ) == 0 )
	{
		movementComponent->direction.x = -1.0f;
	}
	else if ( strcmp( id, "game_strafe_right" ) == 0 )
	{
		movementComponent->direction.x = 1.0f;
	}

	if ( strcmp( id, "game_jump" ) == 0 && movementComponent->isGrounded )
	{
		movementComponent->direction.y = 1.0f;
	}
}

void game_client_actions_register_()
{
	ape_client_input_register_action( "game_say", nullptr, 0, ( ApeInputKey[] ) { 't' }, 1, say_action );

	ape_client_input_register_action( "game_move_forward", nullptr, 0, ( ApeInputKey[] ) { 'w', APE_INPUT_KEY_UP }, 2, move_action );
	ape_client_input_register_action( "game_move_backward", nullptr, 0, ( ApeInputKey[] ) { 's', APE_INPUT_KEY_DOWN }, 2, move_action );
	ape_client_input_register_action( "game_strafe_left", nullptr, 0, ( ApeInputKey[] ) { 'a', APE_INPUT_KEY_LEFT }, 2, move_action );
	ape_client_input_register_action( "game_strafe_right", nullptr, 0, ( ApeInputKey[] ) { 'd', APE_INPUT_KEY_RIGHT }, 2, move_action );

	ape_client_input_register_action( "game_jump", ( ApeInputButton[] ) { INPUT_A }, 1, ( ApeInputKey[] ) { ' ' }, 1, move_action );
}
