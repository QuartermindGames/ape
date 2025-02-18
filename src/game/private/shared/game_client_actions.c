// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"
#include "game_server.h"
#include "components/component_movement.h"

static void say_action( ApeInputState state, const char *id )
{
	//TODO: open chat prompt
}

static void move_action( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	GamePlayer *player = game_server_get_host_player_();
	if ( player == nullptr )
	{
		return;
	}

	ApeEntity *entity = player->entity;
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

	game_print_( "moving: %s\n", id );
}

void game_client_actions_register_()
{
	ape_client_input_register_action( "game_say", nullptr, 0, ( ApeInputKey[] ) { 't' }, 1, say_action );

	ape_client_input_register_action( "game_move_forward", nullptr, 0, ( ApeInputKey[] ) { 'w', APE_INPUT_KEY_UP }, 2, move_action );
	ape_client_input_register_action( "game_move_backward", nullptr, 0, ( ApeInputKey[] ) { 's', APE_INPUT_KEY_DOWN }, 2, move_action );
	ape_client_input_register_action( "game_strafe_left", nullptr, 0, ( ApeInputKey[] ) { 'a', APE_INPUT_KEY_LEFT }, 2, move_action );
	ape_client_input_register_action( "game_strafe_right", nullptr, 0, ( ApeInputKey[] ) { 'd', APE_INPUT_KEY_RIGHT }, 2, move_action );
}
