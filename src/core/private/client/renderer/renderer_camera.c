// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Core camera implementation.

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"
#include "renderer.h"
#include "post/post.h"
#include "legacy/actor.h"
#include "game/game_interface.h"

/* Camera management fun! */

static PLLinkedList *cameras;

static ApeCamera *activeCamera = NULL;

ApeCamera *ar_camera_get_active( void )
{
	return activeCamera;
}

void ar_camera_make_active( ApeCamera *camera )
{
	activeCamera = camera;
}

/****************************************
 ****************************************/

ApeCamera *ar_camera_create( const char *tag, const PLVector3 *position, const PLVector3 *angles )
{
	ApeCamera *camera = PL_NEW( ApeCamera );

	camera->mode = APE_CAMERA_MODE_PERSPECTIVE;
	camera->drawMode = APE_CAMERA_DRAW_MODE_SHADED;

	camera->internal = PlgCreateCamera();
	if ( camera->internal == NULL )
	{
		PRINT_ERROR( "Failed to create camera!\nPL: %s\n", PlGetError() );
	}

	if ( tag != NULL )
	{
		strncpy( camera->tag, tag, sizeof( camera->tag ) - 1 );
	}

	camera->internal->fov = 75.0f;
	camera->internal->far = 1000000.0f;
	apeSetCameraPosition( camera, position );
	apeSetCameraAngles( camera, angles );

	if ( cameras == NULL )
	{
		cameras = PlCreateLinkedList();
		if ( cameras == NULL )
		{
			PRINT_ERROR( "Failed to create cameras list: %s\n", PlGetError() );
		}
	}

	camera->node = PlInsertLinkedListNode( cameras, camera );

	return camera;
}

/**
 * Destroy the given camera. Use this instead
 * of calling PlgDestroyCamera directly, as it
 * will free up user data.
 */
void apeDestroyCamera( ApeCamera *camera )
{
	if ( camera == NULL )
	{
		return;
	}

	PlgDestroyCamera( camera->internal );

	PlDestroyLinkedListNode( camera->node );

	// be sure the global camera gets unset if we're destroying it
	if ( camera == activeCamera )
	{
		activeCamera = NULL;
	}

	PL_DELETE( camera );

	if ( PlGetNumLinkedListNodes( cameras ) == 0 )
	{
		PlDestroyLinkedList( cameras );
		cameras = NULL;
	}
}

void apeSetCameraPosition( ApeCamera *camera, const PLVector3 *position )
{
	camera->internal->position = *position;

	if ( camera->room == NULL )
	{
		ApeWorld *world = apeGetCurrentWorld();
		if ( world == NULL )
		{
			return;
		}

		camera->room = apeGetRoomAtPosition( world, &camera->internal->position );
	}
}

void apeSetCameraAngles( ApeCamera *camera, const PLVector3 *angles )
{
	camera->internal->angles = *angles;
}

PLVector3 apeGetCameraPosition( const ApeCamera *camera )
{
	return camera->internal->position;
}

PLVector3 apeGetCameraAngles( const ApeCamera *camera )
{
	return camera->internal->angles;
}

PLVector3 apeGetCameraForward( const ApeCamera *camera )
{
	return camera->forward;
}

void ar_draw_scene_( ApeCamera *camera, const ApeViewport *viewport );
void apeDrawPerspective_( ApeCamera *camera, ApeViewport *viewport )
{
	if ( camera == NULL )
	{
		camera = ar_camera_get_active();
		if ( camera == NULL )
		{
			return;
		}
	}

	COM_PROFILE_FUNCTION_START();

	int ow, oh;
	PL_GET_CVAR( "r/superSampling", superSampling );
	if ( superSampling != NULL && superSampling->i_value > 1 )
	{
		ow = viewport->width;
		viewport->width *= superSampling->i_value;
		oh = viewport->height;
		viewport->height *= superSampling->i_value;

		PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	}

	PL_GET_CVAR( "r/fov", fov );
	if ( fov != NULL )
	{
		PlgSetCameraFieldOfView( camera->internal, fov->f_value );
	}
	PL_GET_CVAR( "r/near", near );
	if ( near != NULL )
	{
		camera->internal->near = near->f_value;
	}
	PL_GET_CVAR( "r/far", far );
	if ( far != NULL )
	{
		camera->internal->far = far->f_value;
	}

	static const float minHeight = 256.0f;
	static const float maxHeight = 1024.0f;

	PLVector3 angles;
	PLVector3 position;

	/* if we have a parent, follow them */
	Actor *parent = camera->parentActor;
	if ( parent != NULL )
	{
		angles.x = parent->viewPitch;
		angles.y = -parent->angles.y + 90.0f;//-Act_GetAngle( camera->parentActor ) + 90.0f;
		angles.z = 0.0f;

		position = parent->position;
		position.y = Act_GetViewOffset( camera->parentActor );
	}
	else
	{
		angles = camera->internal->angles;
		position = camera->internal->position;
	}

	float speed;
	switch ( camera->mode )
	{
		default:
			break;
		case APE_CAMERA_MODE_PERSPECTIVE:
			camera->internal->angles = angles;
			camera->internal->position = position;
			break;
		case APE_CAMERA_MODE_TOP:
		{
			if ( camera->parentActor != NULL )
			{
				speed = PlVector3Length( camera->parentActor->velocity ) / 16.0f;
				if ( speed > 1.0f )
					speed = 1.0f;
			}
			else
			{
				speed = 0.0f;
			}

			camera->internal->angles.x = -75.0f;
			camera->internal->position = position;
			camera->internal->position.x -= 150.0f;
			camera->internal->position.y += minHeight + PlCosineInterpolate( minHeight, maxHeight, speed );
			break;
		}
	}

	PlgSetupCamera( camera->internal );

	// Draw the scene into a buffer
	ar_draw_scene_( camera, viewport );

	if ( superSampling != NULL && superSampling->i_value > 1 )
	{
		viewport->width = ow;
		viewport->height = oh;

		PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	}

	COM_PROFILE_FUNCTION_END();
}
