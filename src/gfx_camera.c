#include <PL/pl_llist.h>

#include "yin.h"
#include "gfx.h"
#include "act.h"

/* Camera management fun! */

static const char *perspectiveDescriptions[ MAX_VIEW_PERSPECTIVES ] = {
	[ VIEW_PERSPECTIVE_EYE ] = "Eye",
	[ VIEW_PERSPECTIVE_TOP ] = "Top",
	[ VIEW_PERSPECTIVE_SIDE ] = "Side",
	[ VIEW_PERSPECTIVE_FRONT ] = "Front",
};

static PLLinkedList *camerasList = NULL;
static GfxCamera *currentCamera = NULL;

GfxCamera *Gfx_GetCurrentCamera( void ) {
	return currentCamera;
}

GfxCamera *Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles, SysWindow *viewport ) {
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

	return gfxCamera;
}

void Gfx_InitializeCameras( void ) {
	PrintMsg( "Setting up cameras...\n" );

	camerasList = plCreateLinkedList();
	if( camerasList == NULL ) {
		PrintError( "Failed to create camera list!\nPL: %s\n", plGetError() );
	}

#if 0 /* todo: move this out of here! */
	if( Sys_GetLaunchMode() == LAUNCH_MODE_EDITOR ) {
		Gfx_CreateCamera( VIEW_PERSPECTIVE_EYE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
		Gfx_CreateCamera( VIEW_PERSPECTIVE_TOP, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
		Gfx_CreateCamera( VIEW_PERSPECTIVE_SIDE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );
		Gfx_CreateCamera( VIEW_PERSPECTIVE_FRONT, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), true );

		PLLinkedListNode *curNode = plGetRootNode( camerasList );
		GfxCamera *camera = plGetLinkedListNodeUserData( curNode );
		if( camera == NULL ) {
			PrintError( "Uninitialized root node!\n" );
		}

		/* default to first camera added */
		currentCamera = camera;
	}
#endif
}

void Gfx_TickCameras( void ) {
	PLLinkedListNode *curNode = plGetRootNode( camerasList );
	while( curNode != NULL ) {
		GfxCamera *camera = plGetLinkedListNodeUserData( curNode );
		if( camera == NULL ) {
			PrintWarn( "Uninitialized node, skipping!\n" );
			continue;
		}

		if ( camera->viewportPtr == NULL ) {
			PrintWarn( "No viewport assigned to camera, skipping!\n" );
			continue;
		}

		Sys_MakeContextActive( camera->viewportPtr );

		/* if we have a parent, follow them */
		if( camera->parentActor != NULL ) {
			switch( camera->perspective ) {
			default: break;
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
			case VIEW_PERSPECTIVE_TOP:break;
			case VIEW_PERSPECTIVE_SIDE:break;
			case VIEW_PERSPECTIVE_FRONT:break;
			}
		}

		currentCamera = camera;

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
