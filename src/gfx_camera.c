#include <PL/pl_llist.h>
#include <PL/pl_window.h>

#include "yin.h"
#include "gfx.h"
#include "act.h"

/* Camera management fun! */

typedef enum ViewPerspective {
	VIEW_PERSPECTIVE_EYE,

	/* editor modes */
	VIEW_PERSPECTIVE_TOP,	
	VIEW_PERSPECTIVE_SIDE,
	VIEW_PERSPECTIVE_FRONT,

	MAX_VIEW_PERSPECTIVES
} ViewPerspective;
static const char *perspectiveDescriptions[ MAX_VIEW_PERSPECTIVES ] = {
	[ VIEW_PERSPECTIVE_EYE ] = "Eye",
	[ VIEW_PERSPECTIVE_TOP ] = "Top",
	[ VIEW_PERSPECTIVE_SIDE ] = "Side",
	[ VIEW_PERSPECTIVE_FRONT ] = "Front",
};

typedef struct GfxCamera {
	PLWindow			*viewportPtr;		/* unused for now */
	PLCamera			*cameraPtr;			/* the camera used for this viewport */
	ViewPerspective		perspective;
	Actor				*parentActor;
	PLLinkedListNode	*node;				/* node representing this object in the linked list */
} GfxCamera;
static PLLinkedList *camerasList = NULL;
static GfxCamera *currentCamera = NULL;

const PLCamera *Gfx_GetCurrentCamera( void ) {
	return currentCamera->cameraPtr;
}

void Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles, bool createViewport ) {
	u_unused( createViewport );
	
	GfxCamera *gfxCamera = Sys_AllocateMemory( 1, sizeof( GfxCamera ) );

	gfxCamera->cameraPtr = plCreateCamera();
	if( gfxCamera->cameraPtr == NULL ) {
		PrintError( "Failed to create camera!\nPL: %s\n", plGetError() );
	}

	gfxCamera->cameraPtr->viewport.w = YIN_DISPLAY_WIDTH;
	gfxCamera->cameraPtr->viewport.h = YIN_DISPLAY_HEIGHT;

	gfxCamera->perspective = perspective;
	switch( gfxCamera->perspective ) {
	default: PrintError( "Unsupported viewport type %d!\n", gfxCamera->perspective );
	case VIEW_PERSPECTIVE_EYE:
		gfxCamera->cameraPtr->fov = 75.0f;
		break;
	case VIEW_PERSPECTIVE_FRONT:
	case VIEW_PERSPECTIVE_SIDE:
	case VIEW_PERSPECTIVE_TOP:
		gfxCamera->cameraPtr->mode = PL_CAMERA_MODE_ORTHOGRAPHIC;
		gfxCamera->cameraPtr->near = 0.0f;
		gfxCamera->cameraPtr->far = 1000.0f;
		break;
	}

	gfxCamera->cameraPtr->position = position;
	gfxCamera->cameraPtr->angles = angles;

	gfxCamera->node = plInsertLinkedListNode( camerasList, gfxCamera );
}

void Gfx_InitializeCameras( void ) {
	PrintMsg( "Setting up cameras...\n" );

	camerasList = plCreateLinkedList();
	if( camerasList == NULL ) {
		PrintError( "Failed to create camera list!\nPL: %s\n", plGetError() );
	}

	if( Sys_GetLaunchMode() == LAUNCH_MODE_DEFAULT ) {
		Gfx_CreateCamera( VIEW_PERSPECTIVE_EYE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
	} else {
		Gfx_CreateCamera( VIEW_PERSPECTIVE_EYE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
		Gfx_CreateCamera( VIEW_PERSPECTIVE_TOP, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
		Gfx_CreateCamera( VIEW_PERSPECTIVE_SIDE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
		Gfx_CreateCamera( VIEW_PERSPECTIVE_FRONT, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
	}

	currentCamera = plGetRootNode( camerasList );
}

void Gfx_TickCameras( void ) {
	PLLinkedListNode *curNode = plGetRootNode( camerasList );
	while( curNode != NULL ) {
		GfxCamera *camera = plGetLinkedListNodeUserData( curNode );
		if( camera == NULL ) {
			PrintWarn( "Uninitialized node, skipping!\n" );
			continue;
		}

		/* if we have a parent, follow them */
		if( camera->parentActor != NULL ) {
			switch( camera->perspective ) {
			case VIEW_PERSPECTIVE_EYE:
#ifdef DEBUG_CAM
				camera->cameraPtr->position = Act_GetPosition( camera->parentActor );
				camera->cameraPtr->position.y = 512.0f;
				camera->cameraPtr->angles.x = -85.0f;
				camera->cameraPtr->angles.y = -Act_GetAngle( camera->parentActor ) + 90.0f;
#else
				camera->cameraPtr->angles.y = -Act_GetAngle( camera->parentActor ) + 90.0f;
				camera->cameraPtr->position = Act_GetPosition( camera->parentActor );
				camera->cameraPtr->position.y = Act_GetViewOffset( camera->parentActor );
#endif
				break;
			}
		}

		curNode = plGetNextLinkedListNode( curNode );
	}
}

void Gfx_ShutdownCameras( void ) {
	PLLinkedListNode *curNode = plGetRootNode( camerasList );
	while( curNode != NULL ) {
		GfxCamera *camera = plGetLinkedListNodeUserData( curNode );
		if( camera == NULL ) {
			PrintWarn( "Uninitialized node, skipping!\n" );
			continue;
		}

		plDestroyCamera( camera->cameraPtr );
		free( camera );

		curNode = plGetNextLinkedListNode( curNode );
	}

	plDestroyLinkedList( camerasList );
}
