// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_console.h>

#include "ape_private.h"
#include "gui_private.h"

#include "renderer/renderer.h"
#include "renderer/material/material.h"

/****************************************
 * GUI DRAW API
 ****************************************/

typedef struct ApeGuiDrawBatch
{
	PLGMesh     *mesh;
	ApeMaterial *material;
	bool         usedThisFrame;
} ApeGuiDrawBatch;

/****************************************
 * Canvas
 ****************************************/

typedef struct ApeGuiCanvas
{
	ApeRenderTarget *renderTarget;
	bool             filter;
	int              width;
	int              height;

	PLMatrix4 viewMatrix, oldViewMatrix;
	int       oldViewport[ 4 ];
} ApeGuiCanvas;

ApeGuiCanvas *ape_gui_canvas_create( int width, int height )
{
	ApeGuiCanvas *canvas = QM_OS_MEMORY_NEW( ApeGuiCanvas );
	canvas->width        = width;
	canvas->height       = height;
	canvas->renderTarget = ape_render_target_create( "gui", 640, 480, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR, false );
	return canvas;
}

void ape_gui_destroy_canvas( ApeGuiCanvas *canvas )
{
	if ( canvas == NULL )
	{
		return;
	}

	ape_render_target_release( canvas->renderTarget );

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

void ape_gui_get_canvas_size( ApeGuiCanvas *canvas, int *width, int *height )
{
	if ( width != NULL )
	{
		*width = canvas->width;
	}
	if ( height != NULL )
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

static PLGCamera *camera;

static PLLinkedList *batches;

void ape_gui_draw_initialize_()
{
	batches = PlCreateLinkedList();

	camera       = PlgCreateCamera();
	camera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	camera->near = 0.0f;
	camera->far  = 1000.0f;
}

static void destroy_batch( void *user )
{
	ApeGuiDrawBatch *batch = user;
}

void ape_gui_draw_shutdown_()
{
	if ( batches != nullptr )
	{
		PlDestroyLinkedListEx( batches, destroy_batch );
		batches = nullptr;
	}
}

PLGMesh *ape_gui_get_batch_queue_mesh( ApeMaterial *material )
{
	PLLinkedListNode *node = PlGetFirstNode( batches );
	while ( node != NULL )
	{
		ApeGuiDrawBatch *drawBatch = PlGetLinkedListNodeUserData( node );
		if ( drawBatch->material == material )
		{
			return drawBatch->mesh;
		}

		node = PlGetNextLinkedListNode( node );
	}

	// Texture isn't in the queue, so create a new batch request
	ApeGuiDrawBatch *drawBatch = QM_OS_MEMORY_NEW( ApeGuiDrawBatch );
	drawBatch->mesh            = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 256, 256 );
	drawBatch->material        = material;
	PlInsertLinkedListNode( batches, drawBatch );
	return drawBatch->mesh;
}

static void cleanup_batch_queue( void )
{
	PLLinkedListNode *node = PlGetFirstNode( batches );
	while ( node != NULL )
	{
		ApeGuiDrawBatch *drawBatch = PlGetLinkedListNodeUserData( node );
		if ( drawBatch->mesh->num_triangles == 0 )
		{
			PlgDestroyMesh( drawBatch->mesh );

			qm_os_memory_free( drawBatch );

			PLLinkedListNode *prevNode = node;
			node                       = PlGetNextLinkedListNode( node );
			PlDestroyLinkedListNode( prevNode );
			continue;
		}

		PlgClearMesh( drawBatch->mesh );
		node = PlGetNextLinkedListNode( node );
	}

	ape_guiState_.lastNumTriangles = ape_guiState_.numTriangles;
	ape_guiState_.numTriangles     = 0;
	ape_guiState_.lastNumBatches   = ape_guiState_.numBatches;
	ape_guiState_.numBatches       = 0;
}

void gui_canvas_make_active( ApeGuiCanvas *canvas )
{
	// save old state
	int ox, oy, ow, oh;
	PlgGetViewport( &ox, &oy, &ow, &oh );
	canvas->oldViewMatrix = PlgGetViewMatrix();

	PlgSetViewport( 0, 0, canvas->width, canvas->height );

	cleanup_batch_queue();

	ape_render_target_bind( canvas->renderTarget, PLG_FRAMEBUFFER_DRAW );

	PlgSetupCamera( camera );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetTexture( nullptr, 0 );
}

void gui_canvas_display( ApeGuiCanvas *canvas )
{
	ApeGuiDrawBatch *drawBatch;
	COM_ITERATE_LINKED_LIST( drawBatch, batches, i )
	{
		assert( drawBatch != nullptr );

		ape_material_draw( drawBatch->material, drawBatch->mesh, nullptr );

		ape_guiState_.numTriangles += drawBatch->mesh->num_triangles;
		ape_guiState_.numBatches++;
	}

	PlgBindFrameBuffer( nullptr, PLG_FRAMEBUFFER_DRAW );

	// restore
	PlgSetViewMatrix( &canvas->oldViewMatrix );
	PlgSetViewport( canvas->oldViewport[ 0 ], canvas->oldViewport[ 1 ], canvas->oldViewport[ 2 ], canvas->oldViewport[ 3 ] );

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
