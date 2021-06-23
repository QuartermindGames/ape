/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_linkedlist.h>

#include "yin.h"
#include "renderer.h"
#include "actor.h"

/* Camera management fun! */

static const char *perspectiveDescriptions[ MAX_VIEW_PERSPECTIVES ] = {
        [VIEW_PERSPECTIVE_EYE]   = "Eye",
        [VIEW_PERSPECTIVE_TOP]   = "Top",
        [VIEW_PERSPECTIVE_SIDE]  = "Side",
        [VIEW_PERSPECTIVE_FRONT] = "Front",
};

static PLLinkedList *camerasList = NULL;

GfxCamera *Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles )
{
	Print( "Creating %s camera...\n", perspectiveDescriptions[ perspective ] );

	GfxCamera *gfxCamera = globalSystem.MAlloc( sizeof( GfxCamera ), true );

	gfxCamera->internalPtr = PlgCreateCamera();
	if ( gfxCamera->internalPtr == NULL )
		PrintError( "Failed to create camera!\nPL: %s\n", PlGetError() );

	gfxCamera->perspective      = perspective;
	gfxCamera->internalPtr->fov = 75.0f;
	gfxCamera->internalPtr->far = 1000000.0f;
#if 0
	switch ( gfxCamera->perspective ) {
		case VIEW_PERSPECTIVE_FRONT:
		case VIEW_PERSPECTIVE_SIDE:
		case VIEW_PERSPECTIVE_TOP:
			gfxCamera->internalPtr->mode = PLG_CAMERA_MODE_ISOMETRIC;
			gfxCamera->internalPtr->near = 0.0f;
			gfxCamera->internalPtr->far = 1000.0f;
			break;
	}
#endif

	gfxCamera->internalPtr->position = position;
	gfxCamera->internalPtr->angles   = angles;

	gfxCamera->node = PlInsertLinkedListNode( camerasList, gfxCamera );

	return gfxCamera;
}

void Gfx_InitializeCameras( void )
{
	Print( "Initializing cameras...\n" );

	camerasList = PlCreateLinkedList();
	if ( camerasList == NULL )
	{
		PrintError( "Failed to create camera list!\nPL: %s\n", PlGetError() );
	}
}

void R_DrawScene( PLGCamera *camera );
void R_DrawPerspective( GfxCamera *camera )
{
	camera->internalPtr->viewport.w = globalSystem.viewport->w;
	camera->internalPtr->viewport.h = globalSystem.viewport->h;
	camera->internalPtr->viewport.x = globalSystem.viewport->x;
	camera->internalPtr->viewport.y = globalSystem.viewport->y;

	CVar( "graphics.superSampling", superSampling );
	if ( superSampling != NULL )
	{
		camera->internalPtr->viewport.w *= superSampling->i_value;
		camera->internalPtr->viewport.h *= superSampling->i_value;
	}

	/* if we have a parent, follow them */
	if ( camera->parentActor != NULL )
	{
		switch ( camera->perspective )
		{
			default:
				break;
			case VIEW_PERSPECTIVE_EYE:
				camera->internalPtr->angles.x   = Act_GetViewPitch( camera->parentActor );
				camera->internalPtr->angles.y   = -Act_GetAngle( camera->parentActor ) + 90.0f;
				camera->internalPtr->position   = Act_GetPosition( camera->parentActor );
				camera->internalPtr->position.y = Act_GetViewOffset( camera->parentActor );
				break;
			case VIEW_PERSPECTIVE_TOP:
				camera->internalPtr->angles.x = -85.0f;
				camera->internalPtr->angles.y = -Act_GetAngle( camera->parentActor ) + 90.0f;
				camera->internalPtr->position = Act_GetPosition( camera->parentActor );
				camera->internalPtr->position.y += 1024.0f;
				break;
			case VIEW_PERSPECTIVE_SIDE:
			case VIEW_PERSPECTIVE_FRONT:
				break;
		}
	}

	PlgSetupCamera( camera->internalPtr );

	R_DrawScene( camera->internalPtr );
}

void Gfx_ShutdownCameras( void )
{
	PLLinkedListNode *curNode = PlGetFirstNode( camerasList );
	while ( curNode != NULL )
	{
		GfxCamera *camera = PlGetLinkedListNodeUserData( curNode );
		if ( camera == NULL )
		{
			PrintWarn( "Uninitialized node, skipping!\n" );
			continue;
		}

		PlgDestroyCamera( camera->internalPtr );
		globalSystem.Free( camera );

		curNode = PlGetNextLinkedListNode( curNode );
	}

	PlDestroyLinkedList( camerasList );
}
