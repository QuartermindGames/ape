// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_console.h>

#include "ape_private.h"
#include "gui_private.h"

#include "renderer/renderer.h"
#include "renderer/material/material.h"

static PLGCamera *camera;

typedef struct ApeGuiDrawBatch
{
	PLGMesh     *mesh;
	ApeMaterial *material;
	bool         usedThisFrame;
} ApeGuiDrawBatch;

typedef struct ApeGuiCanvas
{
	ApeRenderTarget *renderTarget;
	bool             filter;
	int              width;
	int              height;

	PLMatrix4 viewMatrix, oldViewMatrix;
	int       oldViewport[ 4 ];

	PLLinkedList *batches;
} ApeGuiCanvas;

static ApeGuiCanvas *guiCanvasCurrent;

ApeGuiCanvas *ape_gui_canvas_create( int width, int height )
{
	ApeGuiCanvas *canvas = QM_OS_MEMORY_NEW( ApeGuiCanvas );
	canvas->width        = width;
	canvas->height       = height;
	canvas->renderTarget = ape_render_target_create( "gui", 640, 480, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR, false );

	canvas->batches = PlCreateLinkedList();
	if ( canvas->batches == nullptr )
	{
		ape_error_( true, "Failed to create batch list for GUI: %s\n", PlGetError() );
	}

	return canvas;
}

static void destroy_batch( void *user )
{
	ApeGuiDrawBatch *batch = user;

	if ( batch->mesh != nullptr )
	{
		PlgDestroyMesh( batch->mesh );
	}

	if ( batch->material != nullptr )
	{
		ape_material_release( batch->material );
	}

	qm_os_memory_free( batch );
}

void ape_gui_canvas_destroy( ApeGuiCanvas *canvas )
{
	if ( canvas == nullptr )
	{
		return;
	}

	ape_render_target_release( canvas->renderTarget );

	PlDestroyLinkedListEx( canvas->batches, destroy_batch );

	qm_os_memory_free( canvas );
}

void ape_gui_canvas_set_size( ApeGuiCanvas *canvas, int width, int height )
{
	if ( canvas->width == width && canvas->height == height )
	{
		return;
	}

	ape_render_target_set_size( canvas->renderTarget, width, height );
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
	return ape_render_target_get_texture( canvas->renderTarget );
}

/****************************************
 ****************************************/

void ape_gui_draw_initialize_()
{
	camera       = PlgCreateCamera();
	camera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	camera->near = 0.0f;
	camera->far  = 1000.0f;
}

PLGMesh *get_batch_queue_mesh( ApeGuiCanvas *canvas, ApeMaterial *material )
{
	ApeGuiDrawBatch *drawBatch;
	COM_ITERATE_LINKED_LIST( drawBatch, canvas->batches, i )
	{
		if ( drawBatch->material == material )
		{
			return drawBatch->mesh;
		}
	}

	// texture isn't in the queue, so create a new batch request
	drawBatch           = QM_OS_MEMORY_NEW( ApeGuiDrawBatch );
	drawBatch->mesh     = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 256, 256 );
	drawBatch->material = material;
	PlInsertLinkedListNode( canvas->batches, drawBatch );
	return drawBatch->mesh;
}

static void cleanup_batch_queue( ApeGuiCanvas *canvas )
{
	ApeGuiDrawBatch *drawBatch;
	COM_ITERATE_LINKED_LIST( drawBatch, canvas->batches, i )
	{
		if ( drawBatch->mesh->num_triangles == 0 )
		{
			destroy_batch( drawBatch );

			// remove it from the list
			PlDestroyLinkedListNode( i );
			continue;
		}

		PlgClearMesh( drawBatch->mesh );
	}

	ape_guiState_.lastNumTriangles = ape_guiState_.numTriangles;
	ape_guiState_.numTriangles     = 0;
	ape_guiState_.lastNumBatches   = ape_guiState_.numBatches;
	ape_guiState_.numBatches       = 0;
}

void ape_gui_canvas_make_active( ApeGuiCanvas *canvas )
{
	if ( canvas == nullptr )
	{
		// restore
		PlgSetViewMatrix( &canvas->oldViewMatrix );
		PlgSetViewport( canvas->oldViewport[ 0 ], canvas->oldViewport[ 1 ], canvas->oldViewport[ 2 ], canvas->oldViewport[ 3 ] );

		guiCanvasCurrent = nullptr;
	}

	cleanup_batch_queue( canvas );

	// store old state
	PlgGetViewport( &canvas->oldViewport[ 0 ], &canvas->oldViewport[ 1 ], &canvas->oldViewport[ 2 ], &canvas->oldViewport[ 3 ] );
	canvas->oldViewMatrix = PlgGetViewMatrix();

	ape_setup_2d_viewport_( canvas->width, canvas->height );

	ape_render_target_bind( canvas->renderTarget, PLG_FRAMEBUFFER_DRAW );

	PlgSetupCamera( camera );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

	guiCanvasCurrent = canvas;
}

void ape_gui_canvas_display( ApeGuiCanvas *canvas )
{
	ApeGuiDrawBatch *drawBatch;
	COM_ITERATE_LINKED_LIST( drawBatch, canvas->batches, i )
	{
		assert( drawBatch != nullptr );

		ape_material_draw( drawBatch->material, drawBatch->mesh, nullptr );

		ape_guiState_.numTriangles += drawBatch->mesh->num_triangles;
		ape_guiState_.numBatches++;
	}

	PlgBindFrameBuffer( nullptr, PLG_FRAMEBUFFER_DRAW );

	//printf( "%d tris, %d batches\n", guiState.numTriangles, guiState.numBatches );
}

void ape_gui_draw_filled_rectangle( PLGMesh *mesh, const int x, const int y, const int w, const int h, const int z, const QmMathColour4ub *colour )
{
	assert( mesh->primitive == PLG_MESH_TRIANGLES );

	unsigned int vertices[] = {
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x, y, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x, y + h, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x + w, y, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x + w, y + h, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
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
	QmMathColour4ub bColour = PlColourF32ToU8( colour );

	unsigned int vertices[] = {
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( tl.x, tl.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( tr.x, tr.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( ll.x, ll.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( lr.x, lr.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	};

	PlgAddMeshTriangle( mesh, vertices[ 0 ], vertices[ 1 ], vertices[ 2 ] );
	PlgAddMeshTriangle( mesh, vertices[ 1 ], vertices[ 3 ], vertices[ 2 ] );
}
