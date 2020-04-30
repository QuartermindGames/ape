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

void Gfx_SetCurrentCamera( GfxCamera *camera ) {
	currentCamera = camera;
}

GfxCamera *Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles, SysWindow *viewport ) {
	if ( viewport == NULL ) {
		PrintError( "Invalid viewport!\n" );
	}

	GfxCamera *gfxCamera = Sys_AllocateMemory( 1, sizeof( GfxCamera ) );

	gfxCamera->cameraPtr = plCreateCamera();
	if( gfxCamera->cameraPtr == NULL ) {
		PrintError( "Failed to create camera!\nPL: %s\n", plGetError() );
	}

	gfxCamera->viewportPtr = viewport;
	Sys_GetWindowSize( gfxCamera->viewportPtr, &gfxCamera->cameraPtr->viewport.w, &gfxCamera->cameraPtr->viewport.h );

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
	PrintMsg( "Initializing cameras...\n" );

	camerasList = plCreateLinkedList();
	if( camerasList == NULL ) {
		PrintError( "Failed to create camera list!\nPL: %s\n", plGetError() );
	}
}

void Gfx_DrawScene( GfxCamera *camera );
void Gfx_DrawPerspective( GfxCamera *camera ) {
	if ( camera->viewportPtr == NULL ) {
		PrintWarn( "No viewport assigned to camera, skipping!\n" );
		return;
	}

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
			case VIEW_PERSPECTIVE_TOP:
			case VIEW_PERSPECTIVE_SIDE:
			case VIEW_PERSPECTIVE_FRONT:break;
		}
	}

	Sys_MakeWindowActive( camera->viewportPtr );
	Sys_GetWindowSize( camera->viewportPtr, &camera->cameraPtr->viewport.w, &camera->cameraPtr->viewport.h );

	Gfx_EnableShaderProgram( SHADER_GENERIC );

	plSetupCamera( camera->cameraPtr );

	Gfx_DrawScene( camera );

	/* make sure the rest of the gfx subsystem knows which camera is active... */
	currentCamera = camera;
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
