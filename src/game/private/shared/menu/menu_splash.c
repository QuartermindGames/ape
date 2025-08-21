// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Splash screen, for showing fancy logos on startup.
// Author:  Mark E. Sowden

#include "menu.h"

static constexpr unsigned int MAX_SPLASHES = 4;
static GameMenuSplash         splashScreens[ MAX_SPLASHES ];
static unsigned int           splashNum;
static unsigned int           splashCur;

void game_menu_splash_cleanup_()
{
	for ( unsigned int i = 0; i < splashNum; ++i )
	{
		if ( splashScreens[ i ].p.material != nullptr )
		{
			ape_material_release( splashScreens[ i ].p.material );
			splashScreens[ i ].p.material = nullptr;
		}

		if ( splashScreens[ i ].p.sample != nullptr )
		{
			ape_audio_sample_release( splashScreens[ i ].p.sample );
			splashScreens[ i ].p.sample = nullptr;
		}
	}

	splashCur = splashNum = 0;
}

static void skip_splash( ApeInputState state, const char *id )
{
	game_menu_splash_cleanup_();
}

void game_menu_splash_setup_queue_( const GameMenuSplash *splashes, const unsigned int numSplashes )
{
	if ( PlHasCommandLineArgument( "/skip" ) )
	{
		// if skip intro is specified, queue nothing
		return;
	}

	assert( numSplashes > 0 );
	for ( unsigned int i = 0; i < numSplashes; ++i )
	{
		GameMenuSplash *splash = &splashScreens[ splashNum ];
		*splash                = splashes[ i ];

		splash->p.material = ape_material_cache( splash->materialPath, APE_CACHE_GROUP_GLOBAL, false );
		if ( splash->p.material == nullptr )
		{
			game_warning_( "Failed to load splash screen material (%s)!\n", splash->materialPath );
			continue;
		}

		// the sample is totally optional; if it fails to load,
		// we just throw the warning but otherwise continue
		if ( splash->samplePath != nullptr )
		{
			splash->p.sample = ape_audio_sample_cache( splash->samplePath );
			if ( splash->p.sample == nullptr )
			{
				game_warning_( "Failed to load splash screen sample (%s)!\n", splash->samplePath );
			}
		}

		splash->p.maxLifetime = splash->fadeInTime + splash->fadeOutTime;

		splashNum++;
	}

	ape_client_input_register_action( "game_menu_splash_skip", ( ApeInputButton[] ) { INPUT_A, INPUT_START }, 2, ( ApeInputKey[] ) { APE_INPUT_KEY_ESCAPE }, 1, skip_splash );
}

bool game_menu_splash_is_complete_()
{
	return splashCur >= splashNum;
}

void game_menu_splash_tick_( const double delta )
{
	if ( game_menu_splash_is_complete_() )
	{
		return;
	}

	GameMenuSplash *splash = &splashScreens[ splashCur ];
	if ( splash == nullptr )
	{
		return;
	}

	splash->p.lifetime += 1.0f * delta;
	if ( splash->p.lifetime > splash->p.maxLifetime )
	{
		splashCur++;
	}
}

static void draw_rect( float x, float y, float w, float h, const PLColour *colour, ApeMaterial *material )
{
	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

	PlgImmPushVertex( x, y, 0.0f );
	PlgImmTextureCoord( 0.0f, 0.0f );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );

	PlgImmPushVertex( x, y + h, 0.0f );
	PlgImmTextureCoord( 0.0f, 1.0f );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );

	PlgImmPushVertex( x + w, y, 0.0f );
	PlgImmTextureCoord( 1.0f, 0.0f );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );

	PlgImmPushVertex( x + w, y + h, 0.0f );
	PlgImmTextureCoord( 1.0f, 1.0f );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );

	ape_material_draw( material, mesh, nullptr );
}

void game_menu_splash_draw_( const ApeViewport *viewport )
{
	assert( splashCur < splashNum );

	GameMenuSplash *splash = &splashScreens[ splashCur ];
	if ( splash == nullptr )
	{
		return;
	}

	float w = viewport->width / 3.0f;
	float h = w;
	if ( h > viewport->height )
	{
		w = viewport->height / 3.0f;
		h = w;
	}

	float prog = splash->p.lifetime / splash->p.maxLifetime;
	float fade = 1.0f;
	if ( splash->p.lifetime < splash->fadeInTime )
	{
		fade = splash->p.lifetime / splash->fadeInTime;
	}
	else if ( prog > ( 1.0f - splash->fadeOutTime / splash->p.maxLifetime ) )
	{
		float fadeOutStart = 1.0f - splash->fadeOutTime / splash->p.maxLifetime;
		fade               = 1.0f - ( prog - fadeOutStart ) / ( splash->fadeOutTime / splash->p.maxLifetime );
	}

	draw_rect( viewport->width / 2.0f - w / 2.0f, // x
	           viewport->height / 2.0f - h / 2.0f,// y
	           w, h,
	           &PL_COLOURU8( 255, 255, 255, PlFloatToByte( fade ) ),
	           splash->p.material );
}
