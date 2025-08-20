// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../game_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Splash Screen
/////////////////////////////////////////////////////////////////////////////////////

typedef struct GameMenuSplash
{
	const char *materialPath;// required
	const char *samplePath;  // optional

	float fadeInTime;
	float fadeOutTime;

	struct
	{
		ApeMaterial    *material;
		ApeAudioSample *sample;
		float           lifetime;
	} p;// private data
} GameMenuSplash;

/**
 * Cleanup everything that's queued, essentially skipping.
 */
void game_menu_splash_cleanup_();

/**
 * Queue up all the splash screens you want to show on start-up.
 * @param splashes Array of splash screens you want to display.
 * @param numSplashes The number of splash screens in your array.
 */
void game_menu_splash_setup_queue_( const GameMenuSplash *splashes, unsigned int numSplashes );

/**
 * Check if the splash screens are still displaying or not.
 * @return True if the splash screens are done displaying.
 */
bool game_menu_splash_is_complete_();

/**
 * Simulate the splash screens as they're displayed.
 * (automatically skips if splash is complete).
 * @param delta Delta time.
 */
void game_menu_splash_tick_( double delta );

/**
 * Draw the splash screen.
 * @param viewport Active viewport we're drawing into.
 */
void game_menu_splash_draw_( const ApeViewport *viewport );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
