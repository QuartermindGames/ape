// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "renderer.h"
#include "renderer_render_target.h"

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
static ApeViewport *viewports[ MAX_VIEWPORTS ];
static bool isInitialized = false;

/**
 * Attempts to create a new viewport. Only a maximum of 4 are supported.
 */
ApeViewport *ape_viewport_create( int x, int y, int width, int height, void *windowHandle )
{
	if ( !isInitialized )
	{
		PL_ZERO( viewports, sizeof( ApeViewport * ) * MAX_VIEWPORTS );
		isInitialized = true;
	}

	unsigned int i = 0;
	for ( ; i < MAX_VIEWPORTS; ++i )
	{
		if ( viewports[ i ] != NULL )
			continue;

		break;
	}

	if ( i >= MAX_VIEWPORTS )
	{
		PRINT_WARNING( "Hit maximum viewport limit! Viewport will not be created.\n" );
		return NULL;
	}

	viewports[ i ] = PL_NEW( ApeViewport );
	viewports[ i ]->x = x;
	viewports[ i ]->y = y;
	viewports[ i ]->width = width;
	viewports[ i ]->height = height;
	viewports[ i ]->index = i;
	viewports[ i ]->windowHandle = windowHandle;
	viewports[ i ]->zoom = 1.0f;

#if 1
	char viewportTag[ 64 ];
	snprintf( viewportTag, sizeof( viewportTag ), "viewport_%u", i );
	viewports[ i ]->renderTarget = ape_render_target_create( viewportTag,
	                                                         width, height,
	                                                         PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                                         PLG_BUFFER_COLOUR,
	                                                         PLG_TEXTURE_FILTER_LINEAR );
	if ( viewports[ i ]->renderTarget == NULL )
	{
		PRINT_WARNING( "Failed to create render target for viewport!\n" );
		PL_DELETEN( viewports[ i ] );
	}
#endif

	return viewports[ i ];
}

void ape_viewport_destroy( ApeViewport *viewport )
{
	if ( viewport == NULL )
		return;

	if ( viewport->renderTarget != NULL )
	{
		ape_render_target_release( viewport->renderTarget );
		viewport->renderTarget = NULL;
	}

	unsigned int index = viewport->index;
	PL_DELETE( viewports[ index ] );
	viewports[ index ] = NULL;
}

/**
 * Returns the viewport by the given slot.
 */
ApeViewport *ape_get_viewport_by_slot( unsigned int slot )
{
	assert( slot < MAX_VIEWPORTS );
	if ( slot >= MAX_VIEWPORTS )
	{
		PRINT_WARNING( "Invalid slot specified!\n" );
		return NULL;
	}

	return viewports[ slot ];
}

void ape_viewport_set_camera( ApeViewport *viewport, ApeCamera *camera )
{
	viewport->camera = camera;
}

ApeCamera *ape_viewport_get_camera( ApeViewport *viewport ) { return viewport->camera; }

void ape_viewport_set_size( ApeViewport *viewport, int width, int height )
{
	viewport->width = width;
	viewport->height = height;
}

void ape_viewport_get_size( const ApeViewport *viewport, int *width, int *height )
{
	*width = viewport->width;
	*height = viewport->height;
}

/**
 * Weird one, I know, but frametime is tied in with each viewport...
 */
unsigned int ape_viewport_get_framerate( ApeViewport *viewport )
{
	if ( viewport->perf.frameIndex == 0 )
	{
		double t = 0.0;
		for ( unsigned int i = 0; i < APE_MAX_FPS_READINGS; ++i )
		{
			t += viewport->perf.frameReadings[ i ];
		}

		viewport->perf.lastFramerateUpdate = APE_MAX_FPS_READINGS;
		viewport->perf.lastFramerate = ( unsigned int ) ( t / APE_MAX_FPS_READINGS );
	}

	return viewport->perf.lastFramerate;
}

ApeRenderTarget *ape_viewport_get_render_target( ApeViewport *viewport )
{
	return viewport->renderTarget;
}

void ape_viewport_make_active( ApeViewport *viewport )
{
	ApeRenderTarget *target = ape_viewport_get_render_target( viewport );
	if ( target != NULL )
		ape_render_target_bind( target, PLG_FRAMEBUFFER_DEFAULT );

	PlgClipViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );

	if ( viewport->camera != NULL )
		ape_camera_make_active( viewport->camera );
}
