// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_console.h>

#include "ape_private.h"
#include "gui_private.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_render_target.h"

/****************************************
 * GUI DRAW API
 ****************************************/

typedef struct GUIDrawBatch
{
	PLGMesh *mesh;
	PLGTexture *texture;
	bool usedThisFrame;
} GUIDrawBatch;

/****************************************
 * Canvas
 ****************************************/

typedef struct GuiCanvas
{
	ApeRenderTarget *renderTarget;
	bool filter;
	int width;
	int height;
} GuiCanvas;

GuiCanvas *ape_gui_canvas_create( int width, int height )
{
	GuiCanvas *canvas = PL_NEW( GuiCanvas );
	canvas->width = width;
	canvas->height = height;
	canvas->renderTarget = ape_render_target_create( "gui", 640, 480, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR );
	return canvas;
}

void guiDestroyCanvas( GuiCanvas *canvas )
{
	if ( canvas == NULL )
	{
		return;
	}

	ape_render_target_release( canvas->renderTarget );

	PL_DELETE( canvas );
}

void gui_canvas_set_size( GuiCanvas *canvas, int width, int height )
{
	if ( canvas->width == width && canvas->height == height )
	{
		return;
	}

	ape_render_target_set_size( canvas->renderTarget, width, height );
}

void guiGetCanvasSize( GuiCanvas *canvas, int *width, int *height )
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

PLGTexture *guiGetCanvasTexture( GuiCanvas *canvas )
{
	return ape_render_target_get_texture( canvas->renderTarget );
}

/****************************************
 ****************************************/

static PLGCamera *camera;

static PLLinkedList *batches;

static bool hasBegun = false;

void guiInitializeDraw_( void )
{
	batches = PlCreateLinkedList();

	camera = PlgCreateCamera();
	camera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	camera->near = 0.0f;
	camera->far = 1000.0f;
}

void guiShutdownDraw_( void )
{
}

PLGMesh *guiGetBatchQueueMesh( PLGTexture *texture )
{
	PLLinkedListNode *node = PlGetFirstNode( batches );
	while ( node != NULL )
	{
		GUIDrawBatch *drawBatch = PlGetLinkedListNodeUserData( node );
		if ( drawBatch->texture == texture )
		{
			return drawBatch->mesh;
		}

		node = PlGetNextLinkedListNode( node );
	}

	// Texture isn't in the queue, so create a new batch request
	GUIDrawBatch *drawBatch = PL_NEW( GUIDrawBatch );
	drawBatch->mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 256, 256 );
	drawBatch->texture = texture;
	PlInsertLinkedListNode( batches, drawBatch );
	return drawBatch->mesh;
}

static void CleanupBatchQueue( void )
{
	PLLinkedListNode *node = PlGetFirstNode( batches );
	while ( node != NULL )
	{
		GUIDrawBatch *drawBatch = PlGetLinkedListNodeUserData( node );
		if ( drawBatch->mesh->num_triangles == 0 )
		{
			PlgDestroyMesh( drawBatch->mesh );

			PL_DELETE( drawBatch );

			PLLinkedListNode *prevNode = node;
			node = PlGetNextLinkedListNode( node );
			PlDestroyLinkedListNode( prevNode );
			continue;
		}

		PlgClearMesh( drawBatch->mesh );
		node = PlGetNextLinkedListNode( node );
	}

	guiState.lastNumTriangles = guiState.numTriangles;
	guiState.numTriangles = 0;
	guiState.lastNumBatches = guiState.numBatches;
	guiState.numBatches = 0;
}

void gui_canvas_draw( GuiCanvas *canvas, GuiPanel *root )
{
	COM_PROFILE_FUNCTION_START();

	// save old state
	int ox, oy, ow, oh;
	PlgGetViewport( &ox, &oy, &ow, &oh );
	PLMatrix4 oldViewMatrix = PlgGetViewMatrix();

	PlgSetViewport( 0, 0, canvas->width, canvas->height );

	CleanupBatchQueue();

	ape_render_target_bind( canvas->renderTarget, PLG_FRAMEBUFFER_DRAW );

	PlgSetupCamera( camera );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetTexture( NULL, 0 );

	guiDrawPanel( root );

	PlgSetShaderUniformValue( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ], "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );
	PlgSetShaderUniformValue( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ], "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );

	PLLinkedListNode *node = PlGetFirstNode( batches );
	while ( node != NULL )
	{
		GUIDrawBatch *drawBatch = PlGetLinkedListNodeUserData( node );
		PlgSetTexture( drawBatch->texture, 0 );
		if ( drawBatch->texture == NULL )
		{
			PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
		}
		else
		{
			PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
		}

		PlgUploadMesh( drawBatch->mesh );
		PlgDrawMesh( drawBatch->mesh );

		guiState.numTriangles += drawBatch->mesh->num_triangles;
		guiState.numBatches++;

		node = PlGetNextLinkedListNode( node );
	}

	PlPopMatrix();

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	PlgSetTexture( NULL, 0 );

	// restore
	PlgSetViewMatrix( &oldViewMatrix );
	PlgSetViewport( ox, oy, ow, oh );

	//printf( "%d tris, %d batches\n", guiState.numTriangles, guiState.numBatches );

	COM_PROFILE_FUNCTION_END();
}

void guiDrawFilledRectangle( PLGMesh *mesh, int x, int y, int w, int h, int z, const PLColour *colour )
{
	unsigned int vertices[] = {
	        PlgAddMeshVertex( mesh, &PLVector3( x, y, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &PLVector3( x, y + h, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &PLVector3( x + w, y, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &PLVector3( x + w, y + h, z ), &pl_vecOrigin3, colour, &pl_vecOrigin2 ),
	};

	PlgAddMeshTriangle( mesh, vertices[ 0 ], vertices[ 1 ], vertices[ 2 ] );
	PlgAddMeshTriangle( mesh, vertices[ 1 ], vertices[ 3 ], vertices[ 2 ] );
}

/**
 * Similar to Draw_FilledRectangle, only more explicit for the frame coords.
 */
void guiDrawQuad( PLGMesh *mesh, GUIVector2 tl, GUIVector2 tr, GUIVector2 ll, GUIVector2 lr, int z, const PLColourF32 *colour )
{
	// todo: drawing API should take floating-point colours!
	PLColour bColour = PlColourF32ToU8( colour );

	unsigned int vertices[] = {
	        PlgAddMeshVertex( mesh, &PLVector3( tl.x, tl.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &PLVector3( tr.x, tr.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &PLVector3( ll.x, ll.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	        PlgAddMeshVertex( mesh, &PLVector3( lr.x, lr.y, z ), &pl_vecOrigin3, &bColour, &pl_vecOrigin2 ),
	};

	PlgAddMeshTriangle( mesh, vertices[ 0 ], vertices[ 1 ], vertices[ 2 ] );
	PlgAddMeshTriangle( mesh, vertices[ 1 ], vertices[ 3 ], vertices[ 2 ] );
}
