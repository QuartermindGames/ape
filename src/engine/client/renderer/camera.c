/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_linkedlist.h>

#include "yin.h"
#include "renderer.h"
#include "actor.h"

/* Camera management fun! */

static Camera *globalCamera;
Camera *       R_GetGlobalCamera( void )
{
	if ( globalCamera == NULL )
		globalCamera = R_CreateCamera( &pl_vecOrigin3, &pl_vecOrigin3 );

	return globalCamera;
}

Camera *R_CreateCamera( const PLVector3 *position, const PLVector3 *angles )
{
	Camera *camera = globalSystem.MAlloc( sizeof( Camera ), true );

	camera->followMode = CAMERA_MODE_EYE;

	camera->internal = PlgCreateCamera();
	if ( camera->internal == NULL )
		PrintError( "Failed to create camera!\nPL: %s\n", PlGetError() );

	camera->internal->fov      = 75.0f;
	camera->internal->far      = 1000000.0f;
	camera->internal->position = *position;
	camera->internal->angles   = *angles;

	return camera;
}

/**
 * Destroy the given camera. Use this instead
 * of calling PlgDestroyCamera directly, as it
 * will free up user data.
 */
void R_DestroyCamera( Camera *camera )
{
	if ( camera == NULL )
		return;

	PlgDestroyCamera( camera->internal );

	globalSystem.Free( camera );
}

void RCam_SetPosition( Camera *camera, const PLVector3 *position ) { camera->internal->position = *position; }
PLVector3 RCam_GetPosition( Camera *camera ) { return camera->internal->position; }

void R_DrawScene( Camera *camera );
void R_DrawPerspective( Camera *camera )
{
	camera->internal->viewport.w = globalSystem.viewport->w;
	camera->internal->viewport.h = globalSystem.viewport->h;
	camera->internal->viewport.x = globalSystem.viewport->x;
	camera->internal->viewport.y = globalSystem.viewport->y;

	CVar( "graphics.superSampling", superSampling );
	if ( superSampling != NULL && superSampling->i_value > 1 )
	{
		camera->internal->viewport.w *= superSampling->i_value;
		camera->internal->viewport.h *= superSampling->i_value;
	}

	/* if we have a parent, follow them */
	if ( camera->parentActor != NULL )
	{
		switch ( camera->followMode )
		{
			default: break;
			case CAMERA_MODE_EYE:
				camera->internal->angles.x   = Act_GetViewPitch( camera->parentActor );
				camera->internal->angles.y   = -Act_GetAngle( camera->parentActor ) + 90.0f;
				camera->internal->position   = Act_GetPosition( camera->parentActor );
				camera->internal->position.y = Act_GetViewOffset( camera->parentActor );
				break;
			case CAMERA_MODE_TOPDOWN:
				camera->internal->angles.x = -85.0f;
				camera->internal->angles.y = -Act_GetAngle( camera->parentActor ) + 90.0f;
				camera->internal->position = Act_GetPosition( camera->parentActor );
				camera->internal->position.y += 1024.0f;
				break;
		}
	}

	PlgSetupCamera( camera->internal );

	R_DrawScene( camera );
}
