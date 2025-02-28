// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Core camera implementation.

#include <plcore/pl_hashtable.h>

#include "../ape_private.h"
#include "../renderer/renderer.h"

#include "../world/world.h"

static PLLinkedList *cameras;

void ape_camera_make_active( ApeCamera *camera )
{
	if ( camera != nullptr )
	{
		PlgSetupCamera( camera->internal );
	}

	ape_rendererState_.camera = camera;
}

ApeRoom *ape_camera_get_room( ApeCamera *self )
{
	return ape_world_node_get_room( &self->base );
}

void ape_camera_set_room( ApeCamera *self, ApeRoom *room )
{
	ape_world_node_set_room( &self->base, room );
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

ApeCamera *ape_create_camera( ApeWorldNode *parent, const char *name, const PLVector3 *position, const PLVector3 *angles, ApeCameraViewMode cameraMode, ApeCameraDrawMode drawMode )
{
	ApeCamera *camera = PL_NEW( ApeCamera );
	ape_world_node_setup_( &camera->base, parent, APE_WORLD_NODE_TYPE_CAMERA, name, position, angles );

	camera->mode     = cameraMode;
	camera->drawMode = drawMode;

	camera->internal = PlgCreateCamera();
	if ( camera->internal == nullptr )
	{
		ape_error_( true, "Failed to create camera: %s\n", PlGetError() );
	}

	static const float DEFAULT_FAR  = 1000000.0f;
	static const float DEFAULT_FOV  = 75.0f;
	static const float DEFAULT_NEAR = 0.1f;

	if ( camera->mode == APE_CAMERA_MODE_PERSPECTIVE )
	{
		camera->internal->mode = PLG_CAMERA_MODE_PERSPECTIVE;
		camera->internal->fov  = DEFAULT_FOV;
		camera->internal->far  = DEFAULT_FAR;
		camera->internal->near = DEFAULT_NEAR;
	}
	else if ( camera->mode == APE_CAMERA_MODE_ISOMETRIC )
	{
		camera->internal->mode = PLG_CAMERA_MODE_ISOMETRIC;
		camera->internal->fov  = DEFAULT_FOV;
		camera->internal->far  = DEFAULT_FAR;
		camera->internal->near = DEFAULT_NEAR;
	}
	else
	{
		camera->internal->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
		camera->internal->near = -10000.0f;
		camera->internal->far  = 10000.0f;
	}

	ape_camera_set_position( camera, position );
	ape_camera_set_angles( camera, angles );

	if ( cameras == nullptr )
	{
		cameras = PlCreateLinkedList();
		if ( cameras == nullptr )
		{
			ape_error_( true, "Failed to create cameras list: %s\n", PlGetError() );
		}
	}

	camera->pvs.visitedRooms   = PlCreateHashTable();
	camera->pvs.nodes          = PlCreateVectorArray( 2048 );
	camera->pvs.visibleFaces   = PlCreateVectorArray( 4096 );
	camera->pvs.visiblePortals = PlCreateVectorArray( 1024 );

	camera->node = PlInsertLinkedListNode( cameras, camera );

	return camera;
}

static void cleanup_pvs( ApeCameraVisibleSet *pvs )
{
	PlDestroyHashTable( pvs->visitedRooms );
	PlDestroyVectorArray( pvs->nodes );
	PlDestroyVectorArray( pvs->visibleFaces );
	PlDestroyVectorArray( pvs->visiblePortals );
}

/**
 * Destroy the given camera. Use this instead
 * of calling PlgDestroyCamera directly, as it
 * will free up user data.
 */
void ape_camera_destroy_( void *data, ApeWorldNode *parent )
{
	ApeCamera *self = data;
	if ( self == nullptr )
	{
		return;
	}

	PlgDestroyCamera( self->internal );

	PlDestroyLinkedListNode( self->node );

	// clean up visibility data
	cleanup_pvs( &self->pvs );

	PL_DELETE( self );

	if ( PlGetNumLinkedListNodes( cameras ) == 0 )
	{
		PlDestroyLinkedList( cameras );
		cameras = nullptr;
	}
}

void ape_camera_set_position( ApeCamera *self, const PLVector3 *position )
{
	self->internal->position = *position;
	ape_world_node_set_position( &self->base, position );
}

void ape_camera_set_angles( ApeCamera *camera, const PLVector3 *angles )
{
	camera->internal->angles = *angles;
	ape_world_node_set_angles( &camera->base, angles );
}

PLVector3 ape_camera_get_position( const ApeCamera *camera )
{
	return ape_world_node_get_local_position( &camera->base );
}

PLVector3 ape_camera_get_angles( const ApeCamera *camera )
{
	return ape_world_node_get_angles( &camera->base );
}

PLVector3 ape_camera_get_forward( const ApeCamera *camera )
{
	PLMatrix4 view = camera->internal->internal.view;
	return PL_VECTOR3( view.mm[ 0 ][ 2 ], view.mm[ 1 ][ 2 ], view.mm[ 2 ][ 2 ] );
}

void ape_camera_clear_pvs_( ApeCamera *self );
void ape_camera_build_pvs_( ApeCamera *self );

void ape_draw_scene_( ApeCamera *camera, const ApeViewport *viewport );
void ape_camera_draw_perspective( ApeCamera *camera, const ApeViewport *viewport )
{
	assert( camera != nullptr );

	COM_PROFILE_FUNCTION_START();

	//TODO: ditch this mechanism, probably!
	if ( camera->mode == APE_CAMERA_MODE_ISOMETRIC )
	{
		// Uh, let's hardcode it for this as I can't think why you would want anything else -
		// this is what the other modes are there for!
		camera->internal->angles.x = -35.264f;
	}

#if 0//TODO: old game-specific behaviours, we don't want these anymore!
		case SS_ARL_CAMERA_MODE_TOP:
		{
			static const float minHeight = 256.0f;
			static const float maxHeight = 1024.0f;

#	if 0
			if ( camera->parentActor != nullptr )
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

	PlgSetupCamera( camera->internal );

	ape_camera_clear_pvs_( camera );
	ape_camera_build_pvs_( camera );

	// Draw the scene into a buffer
	ape_draw_scene_( camera, viewport );

	COM_PROFILE_FUNCTION_END();
}

PLGCamera *ape_camera_get_internal( ApeCamera *camera )
{
	return camera->internal;
}

ApeLight **ape_camera_get_visible_lights_( ApeCamera *camera, unsigned int *num )
{
	*num = camera->pvs.numLights;
	return ( ApeLight ** ) camera->pvs.lights;
}

ApeRoom **ape_camera_get_visible_rooms_( ApeCamera *camera, unsigned int *num )
{
	*num = camera->pvs.numRooms;
	return ( ApeRoom ** ) camera->pvs.rooms;
}

ApeWorldNode **ape_camera_get_visible_nodes_( ApeCamera *self, unsigned int *num )
{
	return ( ApeWorldNode ** ) PlGetVectorArrayDataEx( self->pvs.nodes, num );
}

ApeBrushFace **ape_camera_get_visible_portals_( ApeCamera *self, unsigned int *num )
{
	return ( ApeBrushFace ** ) PlGetVectorArrayDataEx( self->pvs.visiblePortals, num );
}

/////////////////////////////////////////////////////////////////////////////////////

static PLVector3 viewPos = { 0.0f, 0.0f, 0.0f };
static int       compare_lights( const void *a, const void *b )
{
	const float da = PlVector3Length( PlSubtractVector3( ( *( ApeLight ** ) a )->base.position, viewPos ) );
	const float db = PlVector3Length( PlSubtractVector3( ( *( ApeLight ** ) b )->base.position, viewPos ) );
	return ( da > db ) ? 1 : -1;
}

static void sort_lights( ApeCamera *camera )
{
	if ( !ape_config_.world.sortLights )
	{
		return;
	}

	viewPos = camera->internal->position;

	qsort( camera->pvs.lights, camera->pvs.numLights, sizeof( ApeLight * ), compare_lights );
}

static void queue_light( ApeCamera *camera, ApeLight *light )
{
	assert( light != nullptr );
	if ( !ape_light_is_active( light ) )
	{
		return;
	}

	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( light ) );
	if ( light->type != APE_LIGHT_TYPE_SUN )
	{
		//TODO: let us configure draw distance per light
		float distance = PlVector3Length( PlSubtractVector3( light->base.position, ape_camera_get_position( camera ) ) );
		if ( distance > ape_config_.renderer.maxLightDistance )
		{
			return;
		}


		const PLCollisionSphere sphere = PlSetupCollisionSphere( pos, light->radius );
		if ( !PlgIsSphereInsideView( camera->internal, &sphere ) )
		{
			return;
		}
	}

	if ( ape_config_.renderer.showLights )
	{
		ape_draw_debug_sphere( pos, PlColourF32ToU8( &light->colour ), light->radius );
		if ( light->type != APE_LIGHT_TYPE_OMNI )
		{
			PLVector3 angles = ape_world_node_get_angles( APE_WORLD_NODE( light ) );
			PLVector3 forward;
			PlAnglesAxes( angles, nullptr, nullptr, &forward );
			PLVector3 end = PlAddVector3( pos, PlScaleVector3F( forward, 16.0f ) );
			ape_draw_debug_arrow( pos, end, PlColourF32ToU8( &light->colour ), 1.0f );
		}
	}

	PL_GET_CVAR( "renderer.testFlares", testFlares );
	if ( testFlares != nullptr && testFlares->b_value )
	{
		pos.z += 16.0f;
		ape_add_flare_to_queue( camera, &pos, &PL_COLOURF32RGB( 1.0f, 0, 1.0f ), 1.0f, light->colour.a );
		pos.z += 16.0f;
		ape_add_flare_to_queue( camera, &pos, &PL_COLOURF32RGB( 1.0f, 0, 1.0f ), 1.0f, 1.0f );
		pos.z += 16.0f;
		ape_add_flare_to_queue( camera, &pos, &PL_COLOURF32RGB( 1.0f, 0, 1.0f ), 1.0f, 1.0f );
	}

	if ( light->flags & APE_LIGHT_FLAG_FLARE )
	{
		//TODO: test the flare is actually visible!!
		ape_add_flare_to_queue( camera, &pos, &PL_COLOURF32RGB( light->colour.r, light->colour.g, light->colour.b ), 1.0f, light->colour.a );
	}

	PlPushBackVectorArrayElement( camera->pvs.nodes, APE_WORLD_NODE( light ) );

	camera->pvs.lights[ camera->pvs.numLights ] = light;
	camera->pvs.numLights++;
}

static void test_node_visibility( ApeCamera *self, ApeWorldNode *node )
{
	if ( ape_config_.world.showNodeVolumes )
	{
		PLCollisionAABB transformedBounds = ape_world_node_get_transformed_local_bounds( node );
		PLMatrix4       transform         = ape_world_node_get_transform( node );
		PLVector3       pos               = PlGetMatrix4Translation( &transform );
		ape_draw_debug_axis( pos, node->angles, 16.0f );
		ape_draw_debug_aabb( &node->bounds, PL_COLOUR_PURPLE );
		ape_draw_debug_aabb( &transformedBounds, PL_COLOUR_ORANGE );
	}

	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		if ( self->pvs.numLights < APE_CAMERA_MAX_VISIBLE_LIGHTS )
		{
			ApeLight *light = ( ApeLight * ) node;
			if ( light != nullptr )
			{
				queue_light( self, light );
			}
		}
		else
		{
			ape_warning_( "Maximum visible light limit reached (%u >= %u)!\n", self->pvs.numLights, APE_CAMERA_MAX_VISIBLE_LIGHTS );
		}
	}
	else if ( PlgIsBoxInsideView( self->internal, &node->bounds ) )
	{
		PlPushBackVectorArrayElement( self->pvs.nodes, node );

		if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
		{
			const ApeBrush *brush = ( ApeBrush * ) node;
			for ( uint j = 0; j < brush->numFaces; ++j )
			{
				ApeBrushFace *face = &brush->faces[ j ];
				if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN )
				{
					continue;
				}

				if ( !PlgIsBoxInsideView( self->internal, &face->bounds ) )
				{
					continue;
				}

				PLVector3 view = PlNormalizeVector3( PlSubtractVector3( ape_camera_get_position( self ), face->bounds.absOrigin ) );
				if ( PlVector3DotProduct( face->normal, view ) < 0.0f )
				{
					continue;
				}

				if ( ape_config_.renderer.showFaceNormals )
				{
					ape_draw_debug_arrow( face->bounds.absOrigin, PlAddVector3( face->bounds.absOrigin, PlScaleVector3F( face->normal, 8.0f ) ), PL_COLOUR_WHITE, 2.0f );
				}

				if ( ape_brush_face_is_portal( face ) )
				{
					ape_rendererPerformance_.numVisiblePortals++;
					PlPushBackVectorArrayElement( self->pvs.visiblePortals, face );
					//TODO: traverse connected room
				}

				PlPushBackVectorArrayElement( self->pvs.visibleFaces, face );
			}
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		test_node_visibility( self, child );
	}
}

static void test_room_visibility( ApeCamera *self, ApeRoom *room )
{
	if ( self->pvs.numRooms >= APE_CAMERA_MAX_VISIBLE_ROOMS )
	{
		ape_warning_( "Maximum visible room limit reached (%u >= %u)!\n", self->pvs.numRooms, APE_CAMERA_MAX_VISIBLE_ROOMS );
		return;
	}

	if ( room->numVisits > 0 )
	{
		return;
	}

	room->numVisits++;

	PlPushMatrix();

	test_node_visibility( self, APE_WORLD_NODE( room ) );

	room->numVisits--;

	self->pvs.rooms[ self->pvs.numRooms ].transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	self->pvs.rooms[ self->pvs.numRooms ].room      = room;
	self->pvs.numRooms++;

	PlPopMatrix();
}

void ape_camera_build_pvs_( ApeCamera *self )
{
	PlClearHashTable( self->pvs.visitedRooms );
	self->pvs.numLights = 0;
	self->pvs.numRooms  = 0;

	ApeRoom *room = ape_world_node_get_room( &self->base );
	if ( room == nullptr )
	{
		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

#if 0
	PLCollisionRay ray = {};
	ray.origin         = PL_VECTOR3( 0.0f, s, -r );
	ray.direction      = PL_VECTOR3( 1.0f, 0.0f, 0.0f );

	ApeRayIntersection intersection = {};
	if ( ape_room_ray_intersect( room, &ray, &intersection ) )
	{
		ape_draw_debug_arrow( ray.origin, intersection.intersection, PL_COLOUR_GREEN, 2.0f );
	}
	else
	{
		ape_draw_debug_arrow( ray.origin, PlAddVector3( ray.origin, PlScaleVector3F( ray.direction, 128.0f ) ), PL_COLOUR_RED, 2.0f );
	}
#endif

	test_room_visibility( self, room );
	sort_lights( self );

	PlPopMatrix();

	ape_rendererPerformance_.numRooms += self->pvs.numRooms;
	//ape_rendererPerformance_.numLights += self->visibility.numLights;
}

void ape_camera_clear_pvs_( ApeCamera *self )
{
	PlClearHashTable( self->pvs.visitedRooms );
	PlClearVectorArray( self->pvs.nodes );
	PlClearVectorArray( self->pvs.visibleFaces );
	PlClearVectorArray( self->pvs.visiblePortals );
	self->pvs.numLights = 0;
	self->pvs.numRooms  = 0;
}

const ApeWorldNodeClass ape_cameraClass = {
        .identifier      = "camera",
        .magic           = PL_MAGIC_TO_NUM( 'C', 'A', 'M', ' ' ),
        .destroyFunction = ape_camera_destroy_,

        .editorIcon = "resources/new_camera.gif",
};
