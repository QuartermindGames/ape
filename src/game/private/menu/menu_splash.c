// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Splash screen, for showing fancy logos on startup.
// Author:  Mark E. Sowden

#include "core/public/ape_video.h"

#include "menu.h"

static constexpr unsigned int MAX_SPLASHES = 4;
static GameMenuSplash         splashScreens[ MAX_SPLASHES ];
static unsigned int           splashNum;
static unsigned int           splashCur;

void game_menu_splash_cleanup_()
{
	for ( unsigned int i = 0; i < splashNum; ++i )
	{
		GameMenuSplash *splash = &splashScreens[ splashNum ];
		if ( splash->type == GAME_MENU_SPLASH_TYPE_IMAGE )
		{
			if ( splash->image.p.material != nullptr )
			{
				ape_material_release( splash->image.p.material );
				splash->image.p.material = nullptr;
			}
			if ( splash->image.p.sample != nullptr )
			{
				ape_audio_sample_release( splash->image.p.sample );
				splash->image.p.sample = nullptr;
			}
		}
		else if ( splash->type == GAME_MENU_SPLASH_TYPE_VIDEO && splash->video.p.video != nullptr )
		{
			ape_video_destroy( splash->video.p.video );
			splash->video.p.video = nullptr;
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

		if ( splash->type == GAME_MENU_SPLASH_TYPE_IMAGE )
		{
			splash->image.p.material = ape_material_cache( splash->image.materialPath, APE_CACHE_GROUP_GLOBAL, false );
			if ( splash->image.p.material == nullptr )
			{
				game_warning_( "Failed to load splash screen material (%s)!\n", splash->image.materialPath );
				continue;
			}

			// the sample is totally optional; if it fails to load,
			// we just throw the warning but otherwise continue
			if ( splash->image.samplePath != nullptr )
			{
				splash->image.p.sample = ape_audio_sample_cache( splash->image.samplePath );
				if ( splash->image.p.sample == nullptr )
				{
					game_warning_( "Failed to load splash screen sample (%s)!\n", splash->image.samplePath );
				}
			}

			splash->image.p.maxLifetime = splash->image.fadeInTime + splash->image.fadeOutTime;
		}
		else if ( splash->type == GAME_MENU_SPLASH_TYPE_VIDEO )
		{
			splash->video.p.video = ape_video_load( splash->video.path );
			if ( splash->video.p.video == nullptr )
			{
				game_warning_( "Failed to load splash screen video (%s)!\n", splash->image.materialPath );
				continue;
			}
		}

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

	if ( splash->type == GAME_MENU_SPLASH_TYPE_IMAGE )
	{
		splash->image.p.lifetime += 1.0f * delta;
		if ( splash->image.p.lifetime > splash->image.p.maxLifetime )
		{
			splashCur++;
		}
	}
	else if ( splash->type == GAME_MENU_SPLASH_TYPE_VIDEO && splash->video.p.video != nullptr )
	{
		ape_video_tick( splash->video.p.video, delta );

		if ( !ape_video_is_playing( splash->video.p.video ) )
		{
			splashCur++;
		}
	}
}

static void draw_rect( const float x, const float y, const float w, const float h, const QmMathColour4ub *colour, ApeMaterial *material )
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

static void splash_draw_image( const ApeViewport *viewport, GameMenuSplashImage *image )
{
	float w = viewport->width / 3.0f;
	float h = w;
	if ( h > viewport->height )
	{
		w = viewport->height / 3.0f;
		h = w;
	}

	float prog = image->p.lifetime / image->p.maxLifetime;
	float fade = 1.0f;
	if ( image->p.lifetime < image->fadeInTime )
	{
		fade = image->p.lifetime / image->fadeInTime;
	}
	else if ( prog > 1.0f - image->fadeOutTime / image->p.maxLifetime )
	{
		float fadeOutStart = 1.0f - image->fadeOutTime / image->p.maxLifetime;
		fade               = 1.0f - ( prog - fadeOutStart ) / ( image->fadeOutTime / image->p.maxLifetime );
	}

	draw_rect( viewport->width / 2.0f - w / 2.0f, // x
	           viewport->height / 2.0f - h / 2.0f,// y
	           w, h,
	           &QM_MATH_COLOUR4UB( 255, 255, 255, QM_MATH_FTOB( fade ) ),
	           image->p.material );
}

void game_menu_splash_draw_( const ApeViewport *viewport )
{
	assert( splashCur < splashNum );

	GameMenuSplash *splash = &splashScreens[ splashCur ];
	if ( splash == nullptr )
	{
		return;
	}

	if ( splash->type == GAME_MENU_SPLASH_TYPE_IMAGE )
	{
		splash_draw_image( viewport, &splash->image );
	}
	else if ( splash->type == GAME_MENU_SPLASH_TYPE_VIDEO && splash->video.p.video != nullptr )
	{
		ape_video_draw( splash->video.p.video, viewport );
	}
}
