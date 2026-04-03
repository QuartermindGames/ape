// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <plcore/pl_console.h>

#include "ape_private.h"
#include "gui_private.h"

#include "renderer/renderer.h"
#include "renderer/renderer_render_target.h"
#include "renderer/material/material.h"

static PLGCamera *camera;

typedef struct ApeGuiCanvas
{
	ApeRenderTarget *renderTarget;
	bool             filter;
	int              width;
	int              height;

	PLMatrix4 viewMatrix, oldViewMatrix;
	int       oldViewport[ 4 ];
} ApeGuiCanvas;

static ApeGuiCanvas *guiCanvasCurrent;

ApeGuiCanvas *ape_gui_canvas_create( int width, int height )
{
	ApeGuiCanvas *canvas = QM_OS_MEMORY_NEW( ApeGuiCanvas );
	canvas->width        = width;
	canvas->height       = height;
	canvas->renderTarget = ape_render_target_create_( "gui", 640, 480, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR, false );

	return canvas;
}

void ape_gui_canvas_destroy( ApeGuiCanvas *canvas )
{
	if ( canvas == nullptr )
	{
		return;
	}

	ape_render_target_release_( canvas->renderTarget );

	qm_os_memory_free( canvas );
}

void ape_gui_canvas_set_size( ApeGuiCanvas *canvas, int width, int height )
{
	if ( canvas->width == width && canvas->height == height )
	{
		return;
	}

	ape_render_target_set_size_( canvas->renderTarget, width, height );
}

void ape_gui_canvas_get_size( const ApeGuiCanvas *canvas, int *width, int *height )
{
	if ( width != nullptr )
	{
		*width = canvas->width;
	}
	if ( height != nullptr )
	{
		*height = canvas->height;
	}
}

PLGTexture *ape_gui_get_canvas_texture( ApeGuiCanvas *canvas )
{
	return ape_render_target_get_texture_( canvas->renderTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR );
}

/****************************************
 ****************************************/

static PLMatrix4 view;
static PLMatrix4 proj;

void ape_gui_canvas_make_active( ApeGuiCanvas *canvas )
{
	if ( canvas == nullptr )
	{
		// restore
		PlgSetViewMatrix( &canvas->oldViewMatrix );
		qm_gfx_set_viewport( canvas->oldViewport[ 0 ], canvas->oldViewport[ 1 ], canvas->oldViewport[ 2 ], canvas->oldViewport[ 3 ] );

		guiCanvasCurrent = nullptr;
	}

	// store old state
	qm_gfx_get_viewport( &canvas->oldViewport[ 0 ], &canvas->oldViewport[ 1 ], &canvas->oldViewport[ 2 ], &canvas->oldViewport[ 3 ] );
	canvas->oldViewMatrix = PlgGetViewMatrix();

	ape_setup_2d_viewport_( canvas->width, canvas->height );

	ape_render_target_bind_( canvas->renderTarget, PLG_FRAMEBUFFER_DRAW );

	//PlgSetupCamera( camera );

	proj = PlOrtho( 0.0f, canvas->width, canvas->height, 0.0f, -10000.0f, 10000.0f );
	PlgSetProjectionMatrix( &proj );

	view = PlMatrix4Identity();
	PlgSetViewMatrix( &view );

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

	guiCanvasCurrent = canvas;
}

void ape_gui_canvas_display( ApeGuiCanvas *canvas )
{
	qm_gfx_framebuffer_bind( nullptr, PLG_FRAMEBUFFER_DRAW );

	//printf( "%d tris, %d batches\n", guiState.numTriangles, guiState.numBatches );
}

void ape_gui_draw_filled_rectangle( PLGMesh *mesh, const int x, const int y, const int w, const int h, const int z, const QmMathColour4ub *colour )
{
	assert( mesh->primitive == PLG_MESH_TRIANGLES );

	unsigned int vertices[] = {
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x, y, z ), &QM_MATH_VECTOR3F_ZERO, colour, &QM_MATH_VECTOR2F_ZERO ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x, y + h, z ), &QM_MATH_VECTOR3F_ZERO, colour, &QM_MATH_VECTOR2F_ZERO ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x + w, y, z ), &QM_MATH_VECTOR3F_ZERO, colour, &QM_MATH_VECTOR2F_ZERO ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x + w, y + h, z ), &QM_MATH_VECTOR3F_ZERO, colour, &QM_MATH_VECTOR2F_ZERO ),
	};

	PlgAddMeshTriangle( mesh, vertices[ 0 ], vertices[ 1 ], vertices[ 2 ] );
	PlgAddMeshTriangle( mesh, vertices[ 1 ], vertices[ 3 ], vertices[ 2 ] );
}

/**
 * Similar to draw_filled_rectangle, only more explicit for the frame coords.
 */
void ape_gui_draw_quad( PLGMesh *mesh, const QmMathVector2i tl, const QmMathVector2i tr, const QmMathVector2i ll, const QmMathVector2i lr, const int z, const QmMathColour4f *colour )
{
	assert( mesh->primitive == PLG_MESH_TRIANGLES );

	// todo: drawing API should take floating-point colours!
	QmMathColour4ub bColour = QM_MATH_COLOUR4F_TO_4UB( *colour );

	unsigned int vertices[] = {
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( tl.x, tl.y, z ), &QM_MATH_VECTOR3F_ZERO, &bColour, &QM_MATH_VECTOR2F_ZERO ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( tr.x, tr.y, z ), &QM_MATH_VECTOR3F_ZERO, &bColour, &QM_MATH_VECTOR2F_ZERO ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( ll.x, ll.y, z ), &QM_MATH_VECTOR3F_ZERO, &bColour, &QM_MATH_VECTOR2F_ZERO ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( lr.x, lr.y, z ), &QM_MATH_VECTOR3F_ZERO, &bColour, &QM_MATH_VECTOR2F_ZERO ),
	};

	PlgAddMeshTriangle( mesh, vertices[ 0 ], vertices[ 1 ], vertices[ 2 ] );
	PlgAddMeshTriangle( mesh, vertices[ 1 ], vertices[ 3 ], vertices[ 2 ] );
}
