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
	ApeLight *lightA = *( ApeLight ** ) a;
	ApeLight *lightB = *( ApeLight ** ) b;

	float da = PlVector3Length( PlSubtractVector3( lightA->position, viewPos ) );
	float db = PlVector3Length( PlSubtractVector3( lightB->position, viewPos ) );

	return ( da > db ) ? 1 : -1;
}

static void sort_lights( const ApeCamera *camera )
{
	if ( !ape_config_.world.sortLights )
		return;

	viewPos = camera->internal->position;

	ApeLight **lights = ( ApeLight ** ) PlGetVectorArrayData( camera->visibleLights );
	unsigned int numLights = PlGetNumVectorArrayElements( camera->visibleLights );
	qsort( lights, numLights, sizeof( ApeLight * ), compare_lights );
}

/**
 * Right now, there's a giant fuck-off list of lights the world provides
 * and no association between the worlds and rooms, so we need to iterate
 * over every single damn light.
 */
static void build_visible_light_list( ApeCamera *camera, ApeWorld *world )
{
	if ( world->lights == NULL )
		return;

	// determine what lights are visible -
	// for now this operates over all the lights in the world, urgh...
	PlClearVectorArray( camera->visibleLights );
	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->lights ); ++i )
	{
		ApeLight *light = PlGetVectorArrayElementAt( world->lights, i );

		if ( !( light->flags & SS_ARL_LIGHT_FLAG_ENABLED ) )
			continue;

		if ( light->type != APE_LIGHT_TYPE_SUN )
		{
			//TODO: let us configure draw distance per light
			float distance = PlVector3Length( PlSubtractVector3( light->position, ape_camera_get_position( camera ) ) );
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

static void build_visible_room_list( ApeCamera *camera, ApeWorld *world )
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_camera_make_active( ApeCamera *camera )
{
	PlgSetupCamera( camera->internal );
}

void ape_camera_assign_world( ApeCamera *camera, ApeWorld *world )
{
	camera->world = world;
}

ApeWorld *ape_camera_get_world( ApeCamera *camera )
{
	return camera->world;
}

void ape_camera_set_draw_mode( ApeCamera *camera, ApeCameraDrawMode drawMode )
{
	camera->drawMode = drawMode;
}

void ape_camera_set_view_mode( ApeCamera *camera, ApeCameraViewMode viewMode )
{
	camera->mode = viewMode;
}

const char *ape_get_camera_draw_mode_label( ApeCameraDrawMode drawMode )
{
	switch ( drawMode )
	{
		default:
			return "invalid draw mode";
		case APE_CAMERA_DRAW_MODE_SHADED:
			return "shaded";
		case APE_CAMERA_DRAW_MODE_SOLID:
			return "solid";
		case APE_CAMERA_DRAW_MODE_TEXTURED:
			return "textured";
		case APE_CAMERA_DRAW_MODE_WIREFRAME:
			return "wireframe";
	}
}

const char *ape_get_camera_view_mode_label( ApeCameraViewMode viewMode )
{
	switch ( viewMode )
	{
		default:
			return "invalid view mode";
		case APE_CAMERA_MODE_PERSPECTIVE:
			return "perspective";
		case APE_CAMERA_MODE_FRONT:
			return "front";
		case APE_CAMERA_MODE_LEFT:
			return "left";
		case APE_CAMERA_MODE_TOP:
			return "top";
	}
}

/****************************************
 ****************************************/

ApeCamera *ape_camera_create( const char *tag, const PLVector3 *position, const PLVector3 *angles, ApeCameraViewMode cameraMode )
{
	ApeCamera *camera = PL_NEW( ApeCamera );

	ape_world_node_setup_header( &camera->header, APE_WORLD_NODE_TYPE_CAMERA );

	camera->mode = cameraMode;
	camera->drawMode = APE_CAMERA_DRAW_MODE_SHADED;

	camera->internal = PlgCreateCamera();
	if ( camera->internal == NULL )
		PRINT_ERROR( "Failed to create camera!\nPL: %s\n", PlGetError() );

	if ( tag != NULL )
		strncpy( camera->tag, tag, sizeof( camera->tag ) - 1 );

	static const float DEFAULT_FAR = 1000000.0f;
	static const float DEFAULT_FOV = 75.0f;
	static const float DEFAULT_NEAR = 0.1f;

	if ( camera->mode == APE_CAMERA_MODE_PERSPECTIVE )
	{
		camera->internal->mode = PLG_CAMERA_MODE_PERSPECTIVE;
		camera->internal->fov = DEFAULT_FOV;
		camera->internal->far = DEFAULT_FAR;
		camera->internal->near = DEFAULT_NEAR;
	}
	else if ( camera->mode == APE_CAMERA_MODE_ISOMETRIC )
	{
		camera->internal->mode = PLG_CAMERA_MODE_ISOMETRIC;
		camera->internal->fov = DEFAULT_FOV;
		camera->internal->far = DEFAULT_FAR;
		camera->internal->near = DEFAULT_NEAR;
	}
	else
	{
		camera->internal->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
		camera->internal->near = -10000.0f;
		camera->internal->far = 10000.0f;
	}

	ape_camera_set_position( camera, position );
	ape_camera_set_angles( camera, angles );

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
void ape_camera_destroy( ApeCamera *camera )
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

void ape_camera_set_position( ApeCamera *camera, const PLVector3 *position )
{
	camera->internal->position = *position;

	if ( camera->world == NULL )
	{
		return;
	}

	//if ( camera->room == NULL )
	//{
	//	camera->room = ape_world_get_room_at_position( camera->world, &camera->internal->position );
	//}
}

void ape_camera_set_angles( ApeCamera *camera, const PLVector3 *angles )
{
	camera->internal->angles = *angles;
}

PLVector3 ape_camera_get_position( const ApeCamera *camera )
{
	return camera->internal->position;
}

PLVector3 ape_camera_get_angles( const ApeCamera *camera )
{
	return camera->internal->angles;
}

PLVector3 ape_camera_get_forward( const ApeCamera *camera )
{
	return camera->forward;
}

void ape_draw_scene_( ApeCamera *camera, const ApeViewport *viewport );
void ape_camera_draw_perspective( ApeCamera *camera, ApeViewport *viewport )
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

	float speed;
	switch ( camera->mode )
	{
		default:
			break;
		case APE_CAMERA_MODE_ISOMETRIC:
		{
			// Uh, let's hardcode it for this as I can't think why you would want anything else -
			// this is what the other modes are there for!
			camera->internal->angles.x = -35.264f;
			break;
		}
#if 0//TODO: old game-specific behaviours, we don't want these anymore!
		case SS_ARL_CAMERA_MODE_TOP:
		{
			static const float minHeight = 256.0f;
			static const float maxHeight = 1024.0f;

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
	ape_draw_scene_( camera, viewport );

	// Always restore the viewport back
	viewport->width = ow;
	viewport->height = oh;
	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );

	COM_PROFILE_FUNCTION_END();
}

PLGCamera *ape_camera_get_internal( ApeCamera *camera )
{
	return camera->internal;
}

ApeLight **ape_camera_get_visible_lights_( ApeCamera *camera, unsigned int *num )
{
	return ( ApeLight ** ) PlGetVectorArrayDataEx( camera->visibleLights, num );
}

ApeWorldRoom **ape_camera_get_visible_rooms_( ApeCamera *camera, unsigned int *num )
{
	return ( ApeWorldRoom ** ) PlGetVectorArrayDataEx( camera->visibleRooms, num );
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_build_camera_visibility_lists_( void )
{
	COM_PROFILE_FUNCTION_START();

	PLLinkedListNode *node = PlGetFirstNode( cameras );
	while ( node != NULL )
	{
		ApeCamera *camera = PlGetLinkedListNodeUserData( node );
		if ( camera->world != NULL )
		{
			build_visible_room_list( camera, camera->world );
			build_visible_light_list( camera, camera->world );
		}
		node = PlGetNextLinkedListNode( node );
	}

	COM_PROFILE_FUNCTION_END();
}

void ape_clear_camera_visibility_lists_( void )
{
	COM_PROFILE_FUNCTION_START();

	PLLinkedListNode *node = PlGetFirstNode( cameras );
	while ( node != NULL )
	{
		ApeCamera *camera = PlGetLinkedListNodeUserData( node );
		PlClearVectorArray( camera->visibleLights );
		PlClearVectorArray( camera->visibleRooms );
		node = PlGetNextLinkedListNode( node );
	}

	COM_PROFILE_FUNCTION_END();
}
