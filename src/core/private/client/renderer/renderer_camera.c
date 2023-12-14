// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Core camera implementation.

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"
#include "renderer.h"

#include "world/world.h"

#include "game/game_interface.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static const unsigned int MAX_VISIBILITY_DEPTH = 256;// we'll go through 256 portals maximum (maybe hook this to a var)

static PLLinkedList *cameras;

static PLVector3 viewPos = { 0.0f, 0.0f, 0.0f };
static int compare_lights( const void *a, const void *b )
{
	SSArlLight *lightA = *( SSArlLight ** ) a;
	SSArlLight *lightB = *( SSArlLight ** ) b;

	float da = PlVector3Length( PlSubtractVector3( lightA->position, viewPos ) );
	float db = PlVector3Length( PlSubtractVector3( lightB->position, viewPos ) );

	return ( da > db ) ? 1 : -1;
}

static void sort_lights( const SSArlCamera *camera )
{
	if ( !ape_config_.level.sortLights )
		return;

	viewPos = camera->internal->position;

	SSArlLight **lights = ( SSArlLight ** ) PlGetVectorArrayData( camera->visibleLights );
	unsigned int numLights = PlGetNumVectorArrayElements( camera->visibleLights );
	qsort( lights, numLights, sizeof( SSArlLight * ), compare_lights );
}

/**
 * Right now, there's a giant fuck-off list of lights the world provides
 * and no association between the worlds and rooms, so we need to iterate
 * over every single damn light.
 */
static void build_visible_light_list( SSArlCamera *camera, ApeWorld *world )
{
	if ( world->lights == NULL )
		return;

	// determine what lights are visible -
	// for now this operates over all the lights in the world, urgh...
	PlClearVectorArray( camera->visibleLights );
	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->lights ); ++i )
	{
		SSArlLight *light = PlGetVectorArrayElementAt( world->lights, i );

		if ( !( light->flags & SS_ARL_LIGHT_FLAG_ENABLED ) )
			continue;

		if ( light->type != APE_LIGHT_TYPE_SUN )
		{
			//TODO: let us configure draw distance per light
			float distance = PlVector3Length( PlSubtractVector3( light->position, ss_arl_camera_get_position( camera ) ) );
			if ( distance > ape_config_.renderer.maxLightDistance )
				continue;

			PLCollisionSphere sphere = PlSetupCollisionSphere( light->position, light->radius );
			if ( !PlgIsSphereInsideView( camera->internal, &sphere ) )
				continue;
		}

		PlPushBackVectorArrayElement( camera->visibleLights, light );
	}

	sort_lights( camera );

	ape_rendererPerformance_.numLights = PlGetNumVectorArrayElements( camera->visibleLights );
}

static void build_visible_room_list( SSArlCamera *camera, ApeWorld *world )
{
	PlClearVectorArray( camera->visibleRooms );

	if ( camera->room == NULL )
		return;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ss_arl_camera_make_active( SSArlCamera *camera )
{
	PlgSetupCamera( camera->internal );
}

void ss_arl_camera_assign_world( SSArlCamera *camera, ApeWorld *world )
{
	camera->world = world;
}

ApeWorld *ss_arl_camera_get_world( SSArlCamera *camera )
{
	return camera->world;
}

void ss_arl_camera_set_draw_mode( SSArlCamera *camera, ApeCameraDrawMode drawMode )
{
	camera->drawMode = drawMode;
}

void ss_arl_camera_set_view_mode( SSArlCamera *camera, SSArlCameraMode viewMode )
{
	camera->mode = viewMode;
}

/****************************************
 ****************************************/

SSArlCamera *ss_arl_camera_create( const char *tag, const PLVector3 *position, const PLVector3 *angles, SSArlCameraMode cameraMode )
{
	SSArlCamera *camera = PL_NEW( SSArlCamera );

	camera->mode = cameraMode;
	camera->drawMode = SS_ARL_CAMERA_DRAW_MODE_SHADED;

	camera->internal = PlgCreateCamera();
	if ( camera->internal == NULL )
		PRINT_ERROR( "Failed to create camera!\nPL: %s\n", PlGetError() );

	if ( tag != NULL )
		strncpy( camera->tag, tag, sizeof( camera->tag ) - 1 );

	static const float DEFAULT_FAR = 1000000.0f;
	static const float DEFAULT_FOV = 75.0f;

	if ( camera->mode == SS_ARL_CAMERA_MODE_PERSPECTIVE )
	{
		camera->internal->mode = PLG_CAMERA_MODE_PERSPECTIVE;
		camera->internal->fov = DEFAULT_FOV;
		camera->internal->far = DEFAULT_FAR;
	}
	else if ( camera->mode == SS_ARL_CAMERA_MODE_ISOMETRIC )
	{
		camera->internal->mode = PLG_CAMERA_MODE_ISOMETRIC;
		camera->internal->fov = DEFAULT_FOV;
		camera->internal->far = DEFAULT_FAR;
	}
	else
	{
		camera->internal->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
		camera->internal->near = -10000.0f;
		camera->internal->far = 10000.0f;
	}

	ss_arl_camera_set_position( camera, position );
	ss_arl_camera_set_angles( camera, angles );

	if ( cameras == NULL )
	{
		cameras = PlCreateLinkedList();
		if ( cameras == NULL )
			PRINT_ERROR( "Failed to create cameras list: %s\n", PlGetError() );
	}

	camera->visibleLights = PlCreateVectorArray( SS_ARL_MAX_LIGHTS_PER_PASS );
	camera->visibleRooms = PlCreateVectorArray( MAX_VISIBILITY_DEPTH );

	camera->node = PlInsertLinkedListNode( cameras, camera );

	return camera;
}

/**
 * Destroy the given camera. Use this instead
 * of calling PlgDestroyCamera directly, as it
 * will free up user data.
 */
void ss_arl_camera_destroy( SSArlCamera *camera )
{
	if ( camera == NULL )
		return;

	PlgDestroyCamera( camera->internal );

	PlDestroyLinkedListNode( camera->node );

	PlDestroyVectorArray( camera->visibleLights );
	PlDestroyVectorArray( camera->visibleRooms );

	PL_DELETE( camera );

	if ( PlGetNumLinkedListNodes( cameras ) == 0 )
	{
		PlDestroyLinkedList( cameras );
		cameras = NULL;
	}
}

void ss_arl_camera_set_position( SSArlCamera *camera, const PLVector3 *position )
{
	camera->internal->position = *position;

	if ( camera->world == NULL )
		return;

	if ( camera->room == NULL )
		camera->room = ss_acl_level_get_room_at_position( camera->world, &camera->internal->position );
}

void ss_arl_camera_set_angles( SSArlCamera *camera, const PLVector3 *angles )
{
	camera->internal->angles = *angles;
}

PLVector3 ss_arl_camera_get_position( const SSArlCamera *camera )
{
	return camera->internal->position;
}

PLVector3 ss_arl_camera_get_angles( const SSArlCamera *camera )
{
	return camera->internal->angles;
}

PLVector3 ss_arl_camera_get_forward( const SSArlCamera *camera )
{
	return camera->forward;
}

void ss_arl_draw_scene_( SSArlCamera *camera, const SSArlViewport *viewport );
void ss_arl_camera_draw_perspective( SSArlCamera *camera, SSArlViewport *viewport )
{
	if ( camera == NULL )
		camera = viewport->camera;
	if ( camera == NULL )
		return;

	COM_PROFILE_FUNCTION_START();

	int ow = viewport->width;
	viewport->width *= ape_config_.renderer.superSampling;
	int oh = viewport->height;
	viewport->height *= ape_config_.renderer.superSampling;
	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );

	PL_GET_CVAR( "r/fov", fov );
	if ( fov != NULL )
		PlgSetCameraFieldOfView( camera->internal, fov->f_value );

	PL_GET_CVAR( "r/near", near );
	if ( near != NULL )
		camera->internal->near = near->f_value;

	PL_GET_CVAR( "r/far", far );
	if ( far != NULL )
		camera->internal->far = far->f_value;

	static const float minHeight = 256.0f;
	static const float maxHeight = 1024.0f;

#if 0
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
#endif

	float speed;
	switch ( camera->mode )
	{
		default:
			break;
		case SS_ARL_CAMERA_MODE_ISOMETRIC:
		{
			// Uh, let's hardcode it for this as I can't think why you would want anything else -
			// this is what the other modes are there for!
			camera->internal->angles.x = -35.264f;
			break;
		}
#if 0//TODO: old game-specific behaviours, we don't want these anymore!
		case SS_ARL_CAMERA_MODE_TOP:
		{
#	if 0
			if ( camera->parentActor != NULL )
			{
				speed = PlVector3Length( camera->parentActor->velocity ) / 16.0f;
				if ( speed > 1.0f )
					speed = 1.0f;
			}
			else
#	endif
			speed = 0.0f;

			camera->internal->angles.x = -75.0f;
			camera->internal->position.x -= 150.0f;
			camera->internal->position.y += minHeight + PlCosineInterpolate( minHeight, maxHeight, speed );
			break;
		}
#endif
	}

	PlgSetupCamera( camera->internal );

	// Draw the scene into a buffer
	ss_arl_draw_scene_( camera, viewport );

	// Always restore the viewport back
	viewport->width = ow;
	viewport->height = oh;
	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );

	COM_PROFILE_FUNCTION_END();
}

PLGCamera *ss_arl_camera_get_internal( SSArlCamera *camera )
{
	return camera->internal;
}

SSArlLight **ss_acl_camera_get_visible_lights_( SSArlCamera *camera, unsigned int *num )
{
	return ( SSArlLight ** ) PlGetVectorArrayDataEx( camera->visibleLights, num );
}

SSAclWorldRoom **ss_acl_camera_get_visible_rooms_( SSArlCamera *camera, unsigned int *num )
{
	return ( SSAclWorldRoom ** ) PlGetVectorArrayDataEx( camera->visibleRooms, num );
}

/////////////////////////////////////////////////////////////////////////////////////

void ss_arl_build_camera_visibility_lists_( void )
{
	PLLinkedListNode *node = PlGetFirstNode( cameras );
	while ( node != NULL )
	{
		SSArlCamera *camera = PlGetLinkedListNodeUserData( node );
		if ( camera->world != NULL )
		{
			build_visible_light_list( camera, camera->world );
			build_visible_room_list( camera, camera->world );
		}
		node = PlGetNextLinkedListNode( node );
	}
}

void ss_arl_clear_camera_visibility_lists_( void )
{
	PLLinkedListNode *node = PlGetFirstNode( cameras );
	while ( node != NULL )
	{
		SSArlCamera *camera = PlGetLinkedListNodeUserData( node );
		PlClearVectorArray( camera->visibleLights );
		PlClearVectorArray( camera->visibleRooms );
		node = PlGetNextLinkedListNode( node );
	}
}
