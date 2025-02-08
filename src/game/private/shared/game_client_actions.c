// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"

static void say_action( ApeInputState state, const char *id )
{
	//TODO: open chat prompt
}

void game_client_actions_register()
{
	ape_client_input_register_action( "game_say", nullptr, 0, ( ApeInputKey[] ) { 't' }, 1, say_action );
}
