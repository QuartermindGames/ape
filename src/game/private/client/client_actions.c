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

	if ( strcmp( id, "game_move_forward" ) == 0 )
	{
		movementComponent->directions[ GAME_MOVEMENT_DIRECTION_FB ] = 1;
	}
	else if ( strcmp( id, "game_move_backward" ) == 0 )
	{
		movementComponent->directions[ GAME_MOVEMENT_DIRECTION_FB ] = -1;
	}
	if ( strcmp( id, "game_move_left" ) == 0 )
	{
		movementComponent->directions[ GAME_MOVEMENT_DIRECTION_LR ] = 1;
	}
	else if ( strcmp( id, "game_move_right" ) == 0 )
	{
		movementComponent->directions[ GAME_MOVEMENT_DIRECTION_LR ] = -1;
	}

	if ( strcmp( id, "game_jump" ) == 0 && movementComponent->isGrounded )
	{
		movementComponent->directions[ GAME_MOVEMENT_DIRECTION_UD ] = 1;
	}
	else if ( strcmp( id, "game_crouch" ) == 0 )
	{
		movementComponent->directions[ GAME_MOVEMENT_DIRECTION_UD ] = -1;
	}

	if ( strcmp( id, "game_move_sprint" ) == 0 )
	{
		movementComponent->shiftModifier = true;
	}
}

void game_client_actions_register_()
{
	ape_client_input_register_action( "game_say", nullptr, 0, ( ApeInputKey[] ) { 't' }, 1, say_action, 0 );

	// player movement

	ape_client_input_register_action( "game_move_forward", nullptr, 0, ( ApeInputKey[] ) { 'w', APE_INPUT_KEY_UP }, 2, move_action, 0 );
	ape_client_input_register_action( "game_move_backward", nullptr, 0, ( ApeInputKey[] ) { 's', APE_INPUT_KEY_DOWN }, 2, move_action, 0 );
	ape_client_input_register_action( "game_move_left", nullptr, 0, ( ApeInputKey[] ) { 'a' }, 1, move_action, 0 );
	ape_client_input_register_action( "game_move_right", nullptr, 0, ( ApeInputKey[] ) { 'd' }, 1, move_action, 0 );

	ape_client_input_register_action( "game_move_sprint", nullptr, 0, ( ApeInputKey[] ) { KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT }, 2, move_action, 0 );

	ape_client_input_register_action( "game_jump", ( ApeInputButton[] ) { INPUT_A }, 1, ( ApeInputKey[] ) { ' ' }, 1, move_action, 0 );
	ape_client_input_register_action( "game_crouch", ( ApeInputButton[] ) { INPUT_LEFT_STICK }, 1, ( ApeInputKey[] ) { 'c' }, 1, move_action, 0 );
}
