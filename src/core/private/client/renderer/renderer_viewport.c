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
static SSArlViewport *viewports[ MAX_VIEWPORTS ];
static bool isInitialized = false;

/**
 * Attempts to create a new viewport. Only a maximum of 4 are supported.
 */
SSArlViewport *ss_arl_viewport_create( int x, int y, int width, int height, void *windowHandle )
{
	if ( !isInitialized )
	{
		PL_ZERO( viewports, sizeof( SSArlViewport * ) * MAX_VIEWPORTS );
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

	viewports[ i ] = PL_NEW( SSArlViewport );
	viewports[ i ]->x = x;
	viewports[ i ]->y = y;
	viewports[ i ]->width = width;
	viewports[ i ]->height = height;
	viewports[ i ]->index = i;
	viewports[ i ]->windowHandle = windowHandle;

	char viewportTag[ 64 ];
	snprintf( viewportTag, sizeof( viewportTag ), "viewport_%u", i );
	viewports[ i ]->renderTarget = ss_arl_render_target_create( viewportTag,
	                                                            width, height,
	                                                            PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                                            PLG_BUFFER_COLOUR,
	                                                            PLG_TEXTURE_FILTER_LINEAR );
	if ( viewports[ i ]->renderTarget == NULL )
	{
		PRINT_WARNING( "Failed to create render target for viewport!\n" );
		PL_DELETEN( viewports[ i ] );
	}

	return viewports[ i ];
}

void ss_arl_viewport_destroy( SSArlViewport *viewport )
{
	if ( viewport == NULL )
		return;

	if ( viewport->renderTarget != NULL )
	{
		ss_arl_render_target_release( viewport->renderTarget );
		viewport->renderTarget = NULL;
	}

	unsigned int index = viewport->index;
	PL_DELETE( viewports[ index ] );
	viewports[ index ] = NULL;
}

/**
 * Returns the viewport by the given slot.
 */
SSArlViewport *ss_arl_get_viewport_by_slot( unsigned int slot )
{
	assert( slot < MAX_VIEWPORTS );
	if ( slot >= MAX_VIEWPORTS )
	{
		PRINT_WARNING( "Invalid slot specified!\n" );
		return NULL;
	}

	return viewports[ slot ];
}

void ss_arl_viewport_set_camera( SSArlViewport *viewport, SSArlCamera *camera )
{
	viewport->camera = camera;
}

SSArlCamera *ss_arl_viewport_get_camera( SSArlViewport *viewport ) { return viewport->camera; }

void ss_arl_viewport_set_size( SSArlViewport *viewport, int width, int height )
{
	viewport->width = width;
	viewport->height = height;
}

void ss_arl_viewport_get_size( const SSArlViewport *viewport, int *width, int *height )
{
	*width = viewport->width;
	*height = viewport->height;
}

/**
 * Weird one, I know, but frametime is tied in with each viewport...
 */
unsigned int ss_arl_viewport_get_framerate( const SSArlViewport *viewport )
{
	double t = 0.0;
	for ( unsigned int i = 0; i < APE_MAX_FPS_READINGS; ++i )
	{
		t += viewport->perf.frameReadings[ i ];
	}

	return ( unsigned int ) ( t / APE_MAX_FPS_READINGS );
}

SSArlRenderTarget *ss_arl_viewport_get_render_target( SSArlViewport *viewport )
{
	return viewport->renderTarget;
}

void ss_arl_viewport_make_active( SSArlViewport *viewport )
{
	SSArlRenderTarget *target = ss_arl_viewport_get_render_target( viewport );
	assert( target != NULL );
	if ( target == NULL )
		return;

	ss_arl_render_target_bind( target, PLG_FRAMEBUFFER_DEFAULT );

	PlgClipViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );

	if ( viewport->camera != NULL )
		ss_arl_camera_make_active( viewport->camera );
}
