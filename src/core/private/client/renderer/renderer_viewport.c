// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "renderer.h"

/**
 * What's a viewport? I hear you ask. Well, I'm glad you did.
 * When anything is drawn, a viewport handle needs to be provided,
 * this essentially represents what we're rendering *to* - i.e. a
 * window. Whenever the shell updates a particular window, it will
 * basically provide a viewport representing that window, which
 * in-turn allows the engine to track the frame-time for that
 * particular window.
 */

#define MAX_VIEWPORTS 16
static SS_Arl_Viewport *viewports[ MAX_VIEWPORTS ];
static bool isInitialized = false;

/**
 * Attempts to create a new viewport. Only a maximum of 4 are supported.
 */
SS_Arl_Viewport *ss_arl_viewport_create( int x, int y, int width, int height, void *windowHandle ) {
	if ( !isInitialized ) {
		PL_ZERO( viewports, sizeof( SS_Arl_Viewport * ) * MAX_VIEWPORTS );
		isInitialized = true;
	}

	unsigned int i = 0;
	for ( ; i < MAX_VIEWPORTS; ++i ) {
		if ( viewports[ i ] != NULL ) {
			continue;
		}

		break;
	}

	if ( i >= MAX_VIEWPORTS ) {
		PRINT_WARNING( "Hit maximum viewport limit! Viewport will not be created.\n" );
		return NULL;
	}

	viewports[ i ] = PL_NEW( SS_Arl_Viewport );
	viewports[ i ]->x = x;
	viewports[ i ]->y = y;
	viewports[ i ]->width = width;
	viewports[ i ]->height = height;
	viewports[ i ]->index = i;
	viewports[ i ]->windowHandle = windowHandle;

	return viewports[ i ];
}

void ss_arl_viewport_destroy( SS_Arl_Viewport *viewport ) {
	if ( viewport == NULL ) {
		return;
	}

	unsigned int index = viewport->index;
	PL_DELETE( viewports[ index ] );
	viewports[ index ] = NULL;
}

/**
 * Returns the viewport by the given slot.
 */
SS_Arl_Viewport *ss_arl_get_viewport_by_slot( unsigned int slot ) {
	assert( slot < MAX_VIEWPORTS );
	if ( slot >= MAX_VIEWPORTS ) {
		PRINT_WARNING( "Invalid slot specified!\n" );
		return NULL;
	}

	return viewports[ slot ];
}

void ss_arl_viewport_set_camera( SS_Arl_Viewport *viewport, SS_Arl_Camera *camera ) {
	viewport->camera = camera;
}

void ss_arl_viewport_set_size( SS_Arl_Viewport *viewport, int width, int height ) {
	viewport->width = width;
	viewport->height = height;
}

void ss_arl_viewport_get_size( const SS_Arl_Viewport *viewport, int *width, int *height ) {
	*width = viewport->width;
	*height = viewport->height;
}

/**
 * Weird one, I know, but frametime is tied in with each viewport...
 */
unsigned int ss_arl_viewport_get_framerate( const SS_Arl_Viewport *viewport ) {
	double t = 0.0;
	for ( unsigned int i = 0; i < APE_MAX_FPS_READINGS; ++i ) {
		t += viewport->perf.frameReadings[ i ];
	}

	return ( unsigned int ) ( t / APE_MAX_FPS_READINGS );
}
