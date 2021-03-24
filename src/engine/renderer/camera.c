#include <PL/pl_llist.h>

#include "yin.h"
#include "renderer.h"
#include "actor.h"

/* Camera management fun! */

static const char *perspectiveDescriptions[ MAX_VIEW_PERSPECTIVES ] = {
	[ VIEW_PERSPECTIVE_EYE ] = "Eye",
	[ VIEW_PERSPECTIVE_TOP ] = "Top",
	[ VIEW_PERSPECTIVE_SIDE ] = "Side",
	[ VIEW_PERSPECTIVE_FRONT ] = "Front",
};

const char *Gfx_GetPerspectiveDescription( ViewPerspective perspective ) {
	return perspectiveDescriptions[ perspective ];
}

static PLLinkedList *camerasList = NULL;

GfxCamera *Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles, SysWindow *viewport ) {
	Print( "Creating %s camera...\n", perspectiveDescriptions[ perspective ] );

	if ( viewport == NULL ) {
		PrintError( "Invalid viewport!\n" );
	}

	GfxCamera *gfxCamera = globalSystem.MAlloc( sizeof( GfxCamera ), true );

	gfxCamera->internalPtr = plCreateCamera();
	if( gfxCamera->internalPtr == NULL ) {
		PrintError( "Failed to create camera!\nPL: %s\n", plGetError() );
	}

	gfxCamera->viewportPtr = viewport;
	globalSystem.GetWindowSize( gfxCamera->viewportPtr, &gfxCamera->internalPtr->viewport.w, &gfxCamera->internalPtr->viewport.h );

	gfxCamera->perspective = perspective;
	switch( gfxCamera->perspective ) {
	default: PrintError( "Unsupported viewport type %d!\n", gfxCamera->perspective );
	case VIEW_PERSPECTIVE_EYE:
		gfxCamera->internalPtr->fov = 75.0f;
		gfxCamera->internalPtr->far = 1000000.0f;
		break;
	case VIEW_PERSPECTIVE_FRONT:
	case VIEW_PERSPECTIVE_SIDE:
	case VIEW_PERSPECTIVE_TOP:
		gfxCamera->internalPtr->mode = PL_CAMERA_MODE_ORTHOGRAPHIC;
		gfxCamera->internalPtr->near = 0.0f;
		gfxCamera->internalPtr->far = 1000.0f;
		break;
	}

	gfxCamera->internalPtr->position = position;
	gfxCamera->internalPtr->angles = angles;

	gfxCamera->node = plInsertLinkedListNode( camerasList, gfxCamera );

	return gfxCamera;
}

void Gfx_InitializeCameras( void ) {
	Print( "Initializing cameras...\n" );

	camerasList = plCreateLinkedList();
	if( camerasList == NULL ) {
		PrintError( "Failed to create camera list!\nPL: %s\n", plGetError() );
	}
}

void Gfx_DrawScene( PLCamera *camera );
void Gfx_DrawPerspective( GfxCamera *camera ) {
	if ( camera->viewportPtr == NULL ) {
		PrintWarn( "No viewport assigned to camera, skipping!\n" );
		return;
	}

	SysWindow *window = Engine_GetMainWindow();
	globalSystem.GetWindowSize( window, &camera->internalPtr->viewport.w, &camera->internalPtr->viewport.h );
	extern PLConsoleVariable *gVarGraphicsSupersampling;
	camera->internalPtr->viewport.w *= gVarGraphicsSupersampling->i_value;
	camera->internalPtr->viewport.h *= gVarGraphicsSupersampling->i_value;

	/* if we have a parent, follow them */
	if( camera->parentActor != NULL ) {
		switch( camera->perspective ) {
			default: break;
			case VIEW_PERSPECTIVE_EYE:
			    camera->internalPtr->angles.x = Act_GetViewPitch( camera->parentActor );
				camera->internalPtr->angles.y = -Act_GetAngle( camera->parentActor ) + 90.0f;
				camera->internalPtr->position = Act_GetPosition( camera->parentActor );
				camera->internalPtr->position.y = Act_GetViewOffset( camera->parentActor );
				break;
			case VIEW_PERSPECTIVE_TOP:
			case VIEW_PERSPECTIVE_SIDE:
			case VIEW_PERSPECTIVE_FRONT:break;
		}
	}

	plSetupCamera( camera->internalPtr );
	Gfx_DrawScene( camera->internalPtr );
}

void Gfx_ShutdownCameras( void ) {
	PLLinkedListNode *curNode = plGetFirstNode( camerasList );
	while( curNode != NULL ) {
		GfxCamera *camera = plGetLinkedListNodeUserData( curNode );
		if( camera == NULL ) {
			PrintWarn( "Uninitialized node, skipping!\n" );
			continue;
		}

		plDestroyCamera( camera->internalPtr );
		free( camera );

		curNode = plGetNextLinkedListNode( curNode );
	}

	plDestroyLinkedList( camerasList );
}
