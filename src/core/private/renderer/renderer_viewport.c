// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

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
static bool         isInitialized = false;

static ApeViewport *activeViewport;

/**
 * Attempts to create a new viewport. Only a maximum of 4 are supported.
 */
ApeViewport *ape_viewport_create( int x, int y, int width, int height, void *windowHandle, bool msaa )
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
		{
			continue;
		}

		break;
	}

	if ( i >= MAX_VIEWPORTS )
	{
		ape_console_warning_( "Hit maximum viewport limit! Viewport will not be created.\n" );
		return nullptr;
	}

	viewports[ i ]               = QM_OS_MEMORY_NEW( ApeViewport );
	viewports[ i ]->x            = x;
	viewports[ i ]->y            = y;
	viewports[ i ]->width        = width;
	viewports[ i ]->height       = height;
	viewports[ i ]->index        = i;
	viewports[ i ]->windowHandle = windowHandle;
	viewports[ i ]->zoom         = 1.0f;

#if 1
	char viewportTag[ 64 ];
	snprintf( viewportTag, sizeof( viewportTag ), "viewport_%u", i );
	viewports[ i ]->renderTarget = ape_render_target_create_( viewportTag,
	                                                         width, height,
	                                                         PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                                         PLG_BUFFER_COLOUR,
	                                                         PLG_TEXTURE_FILTER_LINEAR, msaa );
	if ( viewports[ i ]->renderTarget == NULL )
	{
		ape_console_warning_( "Failed to create render target for viewport!\n" );
		qm_os_memory_free( viewports[ i ] );
		viewports[ i ] = nullptr;
	}
#endif

	return viewports[ i ];
}

void ape_viewport_destroy( ApeViewport *self )
{
	if ( self == NULL )
	{
		return;
	}

	if ( self->renderTarget != NULL )
	{
		ape_render_target_release_( self->renderTarget );
		self->renderTarget = NULL;
	}

	unsigned int index = self->index;
	qm_os_memory_free( viewports[ index ] );
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
		ape_console_warning_( "Invalid slot specified!\n" );
		return NULL;
	}

	return viewports[ slot ];
}

void ape_viewport_set_camera( ApeViewport *self, ApeCamera *camera )
{
	self->camera = camera;
}

ApeCamera *ape_viewport_get_camera( ApeViewport *viewport ) { return viewport->camera; }

static void update_render_target_size( ApeViewport *self )
{
	if ( self->renderTarget == NULL )
	{
		return;
	}

	unsigned int rw, rh;
	ape_render_target_get_size_( self->renderTarget, &rw, &rh );

	unsigned int nw = self->width * ( int ) ape_config_.renderer.framebufferScale;
	unsigned int nh = self->height * ( int ) ape_config_.renderer.framebufferScale;
	if ( nw == rw && nh == rh )
	{
		return;
	}

	ape_render_target_set_size_( self->renderTarget, nw, nh );
}

void ape_viewport_set_size( ApeViewport *self, int width, int height )
{
	self->width  = width;
	self->height = height;

	update_render_target_size( self );
}

void ape_viewport_get_size( const ApeViewport *self, int *width, int *height )
{
	if ( width != NULL )
	{
		*width = self->width;
	}
	if ( height != NULL )
	{
		*height = self->height;
	}
}

/**
 * Weird one, I know, but frametime is tied in with each viewport...
 */
unsigned int ape_viewport_get_framerate( ApeViewport *self )
{
	if ( self->perf.frameIndex == 0 )
	{
		double t = 0.0;
		for ( unsigned int i = 0; i < APE_MAX_FPS_READINGS; ++i )
		{
			t += self->perf.frameReadings[ i ];
		}

		self->perf.lastFramerateUpdate = APE_MAX_FPS_READINGS;
		self->perf.lastFramerate       = ( unsigned int ) ( t / APE_MAX_FPS_READINGS );
	}

	return self->perf.lastFramerate;
}

ApeRenderTarget *ape_viewport_get_render_target( ApeViewport *self )
{
	return self->renderTarget;
}

void ape_viewport_make_active( ApeViewport *self )
{
	ape_render_target_bind_( self->renderTarget, PLG_FRAMEBUFFER_DEFAULT );

	// we've got to do this here again, because set_viewport_size isn't called quite so often via the regular launcher,
	// though to be honest, it probably shouldn't be called as often from the editor either...
	update_render_target_size( self );

	int rw = self->width * ( int ) ape_config_.renderer.framebufferScale;
	int rh = self->height * ( int ) ape_config_.renderer.framebufferScale;
	ape_viewport_set_clip( self );
	PlgSetViewport( self->x, self->y, rw, rh );

	if ( self->camera != NULL )
	{
		ape_camera_make_active( self->camera );
	}

	activeViewport = self;
}

ApeViewport *ape_viewport_get_active( void )
{
	return activeViewport;
}

void ape_viewport_set_clip( const ApeViewport *self )
{
	int rw = self->width * ( int ) ape_config_.renderer.framebufferScale;
	int rh = self->height * ( int ) ape_config_.renderer.framebufferScale;
	PlgClipViewport( self->x, self->y, rw, rh );
}

void ape_viewport_set_clear_colour( ApeViewport *self, const QmMathColour4ub *clearColour )
{
	self->clearColour = *clearColour;
}

QmMathVector3f ape_viewport_convert_screen_to_world( const ApeViewport *self, const int pos[ 2 ], const PLMatrix4 *viewMatrix, const PLMatrix4 *projMatrix )
{
	return PlConvertScreenToWorld( qm_math_vector2f( pos[ 0 ], ( self->height - pos[ 1 ] ) ),
	                               viewMatrix,
	                               projMatrix,
	                               ( int[] ) { self->x, self->y, self->width, self->height } );
}
