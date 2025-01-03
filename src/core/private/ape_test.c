// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Test code for ensuring API functionality.
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "model/model.h"
#include "game/game_public.h"
#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Models
/////////////////////////////////////////////////////////////////////////////////////

#if !defined( NDEBUG )

static void test_model_command( unsigned int argc, char **argv )
{
	ApeWorld *world = ss_game_get_current_world();
	if ( world == nullptr )
	{
		ape_warning_( "No world loaded, please create a world first!\n" );
		return;
	}

	ApeRoom *room = ape_world_get_first_room_( world );
	if ( room == nullptr )
	{
		ape_warning_( "World has no rooms!\n" );
		return;
	}

	ApeWorldNode *modelNode = ape_world_node_get_child_by_name( APE_WORLD_NODE( room ), "test_model" );
	if ( modelNode != nullptr )
	{
		ape_world_node_destroy( modelNode );
		return;
	}

	const char *modelPath = ( argc == 1 ) ? "models/characters/character_test.mdl.n" : argv[ 1 ];
	ape_model_node_create( APE_WORLD_NODE( room ), "test_model", modelPath );
}

#endif

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void ape_test_register_commands_()
{
#if !defined( NDEBUG )

	PlRegisterConsoleCommand( "test_model",
	                          "Test a specific model. The given test model will be drawn into the world.",
	                          -1, test_model_command );

#endif
}
