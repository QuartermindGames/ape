// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
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

	const char *modelPath = ( argc == 1 ) ? "models/elite.mdl.n" : argv[ 1 ];
	testModel             = ape_model_load( modelPath );
	if ( testModel == nullptr )
	{
		ape_warning_( "Failed to load the specified model (%s)!\n", modelPath );
	}
}

#endif

static void draw_model_( ApeCamera *camera, ApeLight *light )
{
#if !defined( NDEBUG )

	if ( testModel == nullptr )
	{
		return;
	}

	PlPushMatrix();
	PlLoadIdentityMatrix();

	ApeModelAnimationState animationState = {};
	ape_model_draw( testModel, &animationState, PlGetMatrix( PL_MODELVIEW_MATRIX ), light );

	PlPopMatrix();

#endif
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void ape_test_draw_( ApeCamera *camera )
{
#if 1
	ape_rendererState_.ambience = PL_COLOURF32( 0.35f, 0.35f, 0.35f, 1.0f );
	draw_model_( camera, nullptr );
	ape_rendererState_.ambience = PL_COLOURF32( 0.0f, 0.0f, 0.0f, 0.0f );

	PlgDepthMask( false );

	ape_rendererState_.overrideBlendMode = true;
	ape_rendererState_.blendModeA        = PLG_BLEND_ONE;
	ape_rendererState_.blendModeB        = PLG_BLEND_ONE;

	uint       numLights;
	ApeLight **lights = ape_camera_get_visible_lights_( camera, &numLights );
	for ( uint i = 0; i < numLights; ++i )
	{
		draw_model_( camera, lights[ i ] );
	}

	ape_rendererState_.overrideBlendMode = false;
	ape_rendererState_.passStage         = APE_RENDERER_PASS_DEFAULT;

	PlgDepthMask( true );
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
