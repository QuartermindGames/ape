// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Test code for ensuring API functionality.
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "model/model.h"
#include "client/renderer/renderer.h"

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

static void draw_model_( ApeCamera *camera )
{
#if !defined( NDEBUG )

	if ( testModel == nullptr )
	{
		return;
	}

	uint       numLights;
	ApeLight **lights = ape_camera_get_visible_lights_( camera, &numLights );

	PlPushMatrix();
	PlLoadIdentityMatrix();

	PlTranslateMatrix( PL_VECTOR3( 15.0f, 0.0f, 5.0f ) );

	ApeModelAnimationState animationState = {};
	ape_model_draw( testModel, &animationState, PlGetMatrix( PL_MODELVIEW_MATRIX ), ( numLights > 0 ) ? lights[ 0 ] : nullptr );

	PlPopMatrix();

#endif
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void ape_test_draw_( ApeCamera *camera )
{
	draw_model_( camera );
}

void ape_test_register_commands_()
{
#if !defined( NDEBUG )

	PlRegisterConsoleCommand( "test_model",
	                          "Test a specific model. The given test model will be drawn into the world.",
	                          -1, test_model_command );

#endif
}
