// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Core camera implementation.

#include <plcore/pl_linkedlist.h>
#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "renderer.h"

#include "world/world.h"

#include "game/game_public.h"

static PLLinkedList *cameras;

void ape_camera_make_active( ApeCamera *camera )
{
	if ( camera != nullptr )
	{
		PlgSetupCamera( camera->internal );
	}

	ape_rendererState_.camera = camera;
}

ApeWorld *ape_camera_get_world( ApeCamera *self )
{
	ApeWorldNode *worldNode = ape_world_node_get_root( &self->base );
	if ( worldNode == nullptr )
	{
		return nullptr;
	}

	assert( ape_world_node_is_valid_( worldNode, APE_WORLD_NODE_TYPE_ROOT ) );
	return ( ApeWorld * ) worldNode;
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

	camera->visibility.visitedRooms = PlCreateHashTable();
	camera->visibility.nodes        = PlCreateVectorArray( 0 );

	camera->node = PlInsertLinkedListNode( cameras, camera );

	return camera;
}

/**
 * Destroy the given camera. Use this instead
 * of calling PlgDestroyCamera directly, as it
 * will free up user data.
 */
void ape_camera_destroy_( void *data )
{
	ApeCamera *self = ( ApeCamera * ) data;
	if ( self == nullptr )
	{
		return;
	}

	PlgDestroyCamera( self->internal );

	PlDestroyLinkedListNode( self->node );

	// clean up visibility data
	PlDestroyHashTable( self->visibility.visitedRooms );
	PlDestroyVectorArray( self->visibility.nodes );

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
	return ape_world_node_get_position( &camera->base );
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

void ape_draw_scene_( ApeCamera *camera, const ApeViewport *viewport );
void ape_camera_draw_perspective( ApeCamera *camera, ApeViewport *viewport )
{
	//TODO: gross fucking hack, aaahhhh!!!
	if ( camera == nullptr )
		camera = viewport->camera;
	if ( camera == nullptr )
		return;

	COM_PROFILE_FUNCTION_START();

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
	}

	PlgSetupCamera( camera->internal );

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
	*num = camera->visibility.numLights;
	return ( ApeLight ** ) camera->visibility.lights;
}

ApeRoom **ape_camera_get_visible_rooms_( ApeCamera *camera, unsigned int *num )
{
	*num = camera->visibility.numRooms;
	return ( ApeRoom ** ) camera->visibility.rooms;
}

ApeWorldNode **ape_camera_get_visible_nodes_( ApeCamera *self, unsigned int *num )
{
	return ( ApeWorldNode ** ) PlGetVectorArrayDataEx( self->visibility.nodes, num );
}

/////////////////////////////////////////////////////////////////////////////////////

static PLVector3 viewPos = { 0.0f, 0.0f, 0.0f };
static int       compare_lights( const void *a, const void *b )
{
	float da = PlVector3Length( PlSubtractVector3( ( *( ApeLight ** ) a )->base.position, viewPos ) );
	float db = PlVector3Length( PlSubtractVector3( ( *( ApeLight ** ) b )->base.position, viewPos ) );
	return ( da > db ) ? 1 : -1;
}

static void sort_lights( ApeCamera *camera )
{
	if ( !ape_config_.world.sortLights )
	{
		return;
	}

	viewPos = camera->internal->position;

	qsort( camera->visibility.lights, camera->visibility.numLights, sizeof( ApeLight * ), compare_lights );
}

static void queue_light( ApeCamera *camera, ApeLight *light )
{
	assert( light != nullptr );
	if ( !ape_light_is_active( light ) )
	{
		return;
	}

	if ( light->type != APE_LIGHT_TYPE_SUN )
	{
		//TODO: let us configure draw distance per light
		float distance = PlVector3Length( PlSubtractVector3( light->base.position, ape_camera_get_position( camera ) ) );
		if ( distance > ape_config_.renderer.maxLightDistance )
		{
			return;
		}

		PLCollisionSphere sphere = PlSetupCollisionSphere( light->base.position, light->radius );
		if ( !PlgIsSphereInsideView( camera->internal, &sphere ) )
		{
			return;
		}
	}

	if ( ape_config_.renderer.showLights )
	{
		PLVector3 pos = ape_light_get_position( light );
		ape_draw_debug_sphere( pos, PlColourF32ToU8( &light->colour ), light->radius );
		if ( light->type != APE_LIGHT_TYPE_OMNI )
		{
			PLVector3 end = PlAddVector3( pos, PlScaleVector3F( light->base.angles, 2.0f ) );
			ape_draw_debug_arrow( pos, end, PlColourF32ToU8( &light->colour ) );
		}
	}

	PL_GET_CVAR( "renderer.testFlares", testFlares );
	if ( testFlares != nullptr && testFlares->b_value )
	{
		PLVector3 pos = light->base.position;
		pos.z += 16.0f;
		ape_add_flare_to_queue( camera, &pos, &PL_COLOURF32RGB( 1.0f, 0, 1.0f ), 1.0f, light->colour.a );
		pos.z += 16.0f;
		ape_add_flare_to_queue( camera, &pos, &PL_COLOURF32RGB( 1.0f, 0, 1.0f ), 1.0f, 1.0f );
		pos.z += 16.0f;
		ape_add_flare_to_queue( camera, &pos, &PL_COLOURF32RGB( 1.0f, 0, 1.0f ), 1.0f, 1.0f );
	}

	if ( light->flags & APE_LIGHT_FLAG_FLARE )
	{
		ape_add_flare_to_queue( camera, &light->base.position, &PL_COLOURF32RGB( light->colour.r, light->colour.g, light->colour.b ), 1.0f, light->colour.a );
	}

	camera->visibility.lights[ camera->visibility.numLights ] = light;
	camera->visibility.numLights++;
}

static void light_vis_navigate_tree( ApeCamera *camera, ApeWorldNode *node )
{
	if ( camera->visibility.numLights >= APE_CAMERA_MAX_VISIBLE_LIGHTS )
	{
		ape_warning_( "Maximum visible light limit reached (%u >= %u)!\n", camera->visibility.numLights, APE_CAMERA_MAX_VISIBLE_LIGHTS );
		return;
	}

	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		ApeLight *light = ( ApeLight * ) node;
		if ( light != nullptr )
		{
			queue_light( camera, light );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		light_vis_navigate_tree( camera, child );
	}
}

/**
 * Right now, there's a giant fuck-off list of lights the world provides
 * and no association between the worlds and rooms, so we need to iterate
 * over every single damn light.
 */
static void build_visible_light_list( ApeCamera *camera, ApeWorldNode *root )
{
	// determine what lights are visible -
	// for now this operates over all the lights in the world, urgh...
	camera->visibility.numLights = 0;

	ape_clear_flare_queue_();

	light_vis_navigate_tree( camera, root );

	sort_lights( camera );
}

static void test_room_visibility( ApeCameraVisibleSet *visibleSet, PLGCamera *camera, ApeRoom *room )
{
	if ( visibleSet->numRooms >= APE_CAMERA_MAX_VISIBLE_ROOMS )
	{
		ape_warning_( "Maximum visible room limit reached (%u >= %u)!\n", visibleSet->numRooms, APE_CAMERA_MAX_VISIBLE_ROOMS );
		return;
	}

	//TODO: get rid of this and use the node boundary instead per the caller!
	if ( !PlgIsBoxInsideView( camera, &room->base.bounds ) )
	{
		return;
	}

	if ( room->numVisits > 0 )
	{
		return;
	}

	room->numVisits++;

	PlPushMatrix();

	unsigned int   numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		const ApeWorldFace        *face       = faces[ i ];
		static const ApeWorldFace *portalFace = nullptr;
		if ( portalFace != nullptr && face == portalFace )
		{
			continue;
		}

		if ( ape_config_.renderer.showFaceNormals )
		{
			ape_draw_debug_arrow( face->origin, PlAddVector3( face->origin, PlScaleVector3F( face->normal, 0.5f ) ), PL_COLOUR_CRIMSON );
		}

		PLVector3 forward;
		PlAnglesAxes( camera->angles, nullptr, nullptr, &forward );
		if ( !PlgIsBoxInsideView( camera, &face->bounds ) || PlVector3DotProduct( forward, face->normal ) > 0.f )
		{
			continue;
		}

		if ( !ape_world_face_is_portal( face ) )
		{
			continue;
		}

		ApeRoom *nextRoom;
		if ( ape_world_face_is_mirror( face ) )
		{
			nextRoom = room;
		}
		else
		{
			nextRoom = face->portal->roomB;
		}

		portalFace = face;
		test_room_visibility( visibleSet, camera, nextRoom );

		ape_rendererPerformance_.numVisiblePortals++;
	}

	room->numVisits--;

	visibleSet->rooms[ visibleSet->numRooms ].transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	visibleSet->rooms[ visibleSet->numRooms ].room      = room;
	visibleSet->numRooms++;

	PlPopMatrix();
}

void ape_camera_build_visibility_lists_( ApeCamera *self )
{
	PlClearHashTable( self->visibility.visitedRooms );
	self->visibility.numLights = 0;
	self->visibility.numRooms  = 0;

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	ApeRoom *room = ape_world_node_get_room( &self->base );
	if ( room != nullptr && !ape_config_.world.showAllRooms )
	{
		if ( ape_config_.world.showNodeVolumes || ape_editor_get_active_instance() != nullptr )
		{
			ape_draw_debug_aabb( &room->base.bounds, PL_COLOUR_CRIMSON );
		}

		test_room_visibility( &self->visibility, self->internal, room );
		build_visible_light_list( self, ( ApeWorldNode * ) room );
	}
	else if ( ape_config_.world.showAllRooms )
	{
		ApeWorldNode *rootNode = ape_world_node_get_root( &self->base );
		if ( rootNode == nullptr )
		{
			return;
		}

		ApeWorldNode *child;
		COM_ITERATE_LINKED_LIST( child, rootNode->children, i )
		{
			if ( child->type != APE_WORLD_NODE_TYPE_ROOM )
			{
				continue;
			}

			room = ( ApeRoom * ) child;
			if ( room != nullptr )
			{
				if ( ape_config_.world.showNodeVolumes )
				{
					ape_draw_debug_aabb( &room->base.bounds, PL_COLOUR_CRIMSON );
				}

				// this just draws every room we can see from the
				// root though will be incredibly costly for more
				// complex scenes, so best avoid!!!
				test_room_visibility( &self->visibility, self->internal, room );
			}
		}

		build_visible_light_list( self, rootNode );
	}

	PlPopMatrix();

	ape_rendererPerformance_.numRooms += self->visibility.numRooms;
	ape_rendererPerformance_.numLights += self->visibility.numLights;
}

void ape_build_camera_visibility_lists_( void )
{
	COM_PROFILE_FUNCTION_START();

	ape_rendererPerformance_.numRooms          = 0;
	ape_rendererPerformance_.numLights         = 0;
	ape_rendererPerformance_.numVisiblePortals = 0;

	ApeCamera *camera;
	COM_ITERATE_LINKED_LIST( camera, cameras, i )
	{
		ape_camera_build_visibility_lists_( camera );
	}

	COM_PROFILE_FUNCTION_END();
}

void ape_clear_camera_visibility_lists_( void )
{
	COM_PROFILE_FUNCTION_START();

	ape_rendererPerformance_.numRooms          = 0;
	ape_rendererPerformance_.numLights         = 0;
	ape_rendererPerformance_.numVisiblePortals = 0;

	ApeCamera *camera;
	COM_ITERATE_LINKED_LIST( camera, cameras, i )
	{
		PlClearHashTable( camera->visibility.visitedRooms );
		camera->visibility.numLights = 0;
		camera->visibility.numRooms  = 0;
	}

	COM_PROFILE_FUNCTION_END();
}
