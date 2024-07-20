// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Test code for ensuring API functionality.
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "model/model.h"

/////////////////////////////////////////////////////////////////////////////////////
// Models
/////////////////////////////////////////////////////////////////////////////////////

#if !defined( NDEBUG )

static ApeModel *testModel;

static void test_model_command( unsigned int argc, char **argv )
{
	// if no argument provided and a test model is active, release it
	if ( argc == 1 && testModel )
	{
		ape_model_release( testModel );
		testModel = nullptr;
		return;
	}

	const char *modelPath = ( argc == 1 ) ? "models/p_char_a.mdl.n" : argv[ 1 ];
	testModel             = ape_model_load( modelPath );
	if ( testModel == nullptr )
	{
		ape_warning_( "Failed to load the specified model (%s)!\n", modelPath );
	}
}

#endif

void ape_test_draw_model_()
{
#if !defined( NDEBUG )
	if ( testModel == nullptr )
	{
		return;
	}

	PLMatrix4              transform      = PlMatrix4Identity();
	ApeModelAnimationState animationState = {};
	ape_model_draw( testModel, &animationState, &transform );
#endif
}

void ape_test_register_commands_()
{
#if !defined( NDEBUG )
	PlRegisterConsoleCommand( "test_model",
	                          "Test a specific model. The given test model will be drawn into the world.",
	                          -1, test_model_command );
#endif
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
