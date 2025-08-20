// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Core camera implementation.

#include <plcore/pl_hashtable.h>

#include "ape_private.h"

#include "camera.h"

#include "renderer/renderer.h"
#include "world/world.h"

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

	static constexpr float DEFAULT_FAR  = 1000000.0f;
	static constexpr float DEFAULT_FOV  = 75.0f;
	static constexpr float DEFAULT_NEAR = 0.1f;

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

	camera->node = PlInsertLinkedListNode( cameras, camera );

	return camera;
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

	PlgSetupCamera( camera->internal );

	// Draw the scene into a buffer
	ape_draw_scene_( camera, viewport );

	COM_PROFILE_FUNCTION_END();
}

PLGCamera *ape_camera_get_internal( ApeCamera *camera )
{
	return camera->internal;
}

const ApeWorldNodeClass ape_cameraClass = {
        .identifier = "camera",
        .magic      = QM_OS_MAGIC_TO_NUM( 'C', 'A', 'M', ' ' ),
        .destroy    = ape_camera_destroy_,

        .editorIcon = "resources/new_camera.gif",
};

/////////////////////////////////////////////////////////////////////////////////////
// PVS Generation

static void pvs_navigate_node_tree( ApeCamera *self, const ApeViewport *viewport, ApeWorldNode *worldNode, ApeCameraVisibleRoom *visibleRoom );

static PLVector4 get_face_screen_rect( const ApeBrushFace *face, const ApeCamera *camera, const ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	PLMatrix4 view     = camera->internal->internal.view;
	PLMatrix4 proj     = camera->internal->internal.proj;
	PLMatrix4 viewProj = PlMultiplyMatrix4( &proj, &view );

	PLVector4 rect = PL_VECTOR4( viewport->width, viewport->height, 0.0f, 0.0f );

	// get the transform
	ApeBrush *brush = face->parent;
	assert( brush != nullptr );
	PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( brush ) );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		PLVector3 vertex = PlTransformVector3( face->vertices[ i ].position, &transform );

		float     depth;
		PLVector2 screenPos = PlConvertWorldToScreen( &vertex, &viewProj, ( int[] ) { 0, 0, viewport->width, viewport->height }, &depth, true );

		if ( screenPos.x < rect.x )
		{
			rect.x = PlClamp( 0.0f, screenPos.x, viewport->width );
		}
		if ( screenPos.x > rect.z )
		{
			rect.z = PlClamp( 0.0f, screenPos.x, viewport->width );
		}

		// sigh... we need to flip it, again
		screenPos.y = viewport->height - screenPos.y;
		if ( screenPos.y < rect.y )
		{
			rect.y = PlClamp( 0.0f, screenPos.y, viewport->height );
		}
		if ( screenPos.y > rect.w )
		{
			rect.w = PlClamp( 0.0f, screenPos.y, viewport->height );
		}
	}

	rect.z -= rect.x;
	rect.w -= rect.y;

	COM_PROFILE_FUNCTION_END();

	return rect;
}

static bool pvs_test_light( ApeCamera *self, ApeLight *light )
{
	if ( !ape_light_is_active( light ) )
	{
		return false;
	}

	PLVector3 position = ape_light_get_position( light );
	if ( light->type != APE_LIGHT_TYPE_SUN )
	{
		float distance = PlVector3Length( PlSubtractVector3( position, ape_camera_get_position( self ) ) );
		if ( distance > ape_config_.renderer.maxLightDistance )
		{
			return false;
		}

		const PLCollisionSphere sphere = PlSetupCollisionSphere( position, light->radius );
		if ( !PlgIsSphereInsideView( self->internal, &sphere ) )
		{
			return false;
		}
	}

	if ( ape_config_.renderer.showLights )
	{
		ApeRoom *room = ape_camera_get_room( self );
		if ( room == ape_world_node_get_room( APE_WORLD_NODE( light ) ) )
		{
			ape_draw_debug_sphere( position, PlColourF32ToU8( &light->colour ), light->radius );
			if ( light->type != APE_LIGHT_TYPE_OMNI )
			{
				PLVector3 angles = ape_world_node_get_angles( APE_WORLD_NODE( light ) );
				PLVector3 forward;
				PlAnglesAxes( angles, nullptr, nullptr, &forward );
				PLVector3 end = PlAddVector3( position, PlScaleVector3F( forward, 16.0f ) );
				ape_draw_debug_arrow( position, end, PlColourF32ToU8( &light->colour ), 1.0f );
			}
		}
	}

	// for now, for simplicity-sake, flares only work so long as the camera is in the same room
	ApeRoom *room = ape_camera_get_room( self );
	if ( ( light->flags & APE_LIGHT_FLAG_FLARE ) && room == ape_world_node_get_room( APE_WORLD_NODE( light ) ) )
	{
		//TODO: test the flare is actually visible!!
		ape_add_flare_to_queue( self, &position, &PL_COLOURF32RGB( light->colour.r, light->colour.g, light->colour.b ), 1.0f, light->colour.a );
	}

	return true;
}

static void setup_reflection_matrix( const PLVector3 *normal, const PLVector3 *planePoint, PLMatrix4 *reflectionMatrix )
{
	const float d = -PlVector3DotProduct( *normal, *planePoint );

	reflectionMatrix->mm[ 0 ][ 0 ] = 1.0f - 2.0f * normal->x * normal->x;
	reflectionMatrix->mm[ 0 ][ 1 ] = -2.0f * normal->x * normal->y;
	reflectionMatrix->mm[ 0 ][ 2 ] = -2.0f * normal->x * normal->z;
	reflectionMatrix->mm[ 0 ][ 3 ] = 0.0f;

	reflectionMatrix->mm[ 1 ][ 0 ] = -2.0f * normal->y * normal->x;
	reflectionMatrix->mm[ 1 ][ 1 ] = 1.0f - 2.0f * normal->y * normal->y;
	reflectionMatrix->mm[ 1 ][ 2 ] = -2.0f * normal->y * normal->z;
	reflectionMatrix->mm[ 1 ][ 3 ] = 0.0f;

	reflectionMatrix->mm[ 2 ][ 0 ] = -2.0f * normal->z * normal->x;
	reflectionMatrix->mm[ 2 ][ 1 ] = -2.0f * normal->z * normal->y;
	reflectionMatrix->mm[ 2 ][ 2 ] = 1.0f - 2.0f * normal->z * normal->z;
	reflectionMatrix->mm[ 2 ][ 3 ] = 0.0f;

	reflectionMatrix->mm[ 3 ][ 0 ] = -2.0f * normal->x * d;
	reflectionMatrix->mm[ 3 ][ 1 ] = -2.0f * normal->y * d;
	reflectionMatrix->mm[ 3 ][ 2 ] = -2.0f * normal->z * d;
	reflectionMatrix->mm[ 3 ][ 3 ] = 1.0f;
}

static bool pvs_test_brush( ApeCamera *self, const ApeViewport *viewport, ApeBrush *brush, ApeCameraVisibleRoom *visibleRoom )
{
	PLCollisionAABB bounds = ape_world_node_get_transformed_local_bounds( APE_WORLD_NODE( brush ) );
	if ( !PlgIsBoxInsideView( self->internal, &bounds ) )
	{
		return false;
	}

	unsigned int numVisibleFaces = 0;
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		if ( brush->faces[ i ].flags & APE_BRUSH_FACE_FLAG_HIDDEN )
		{
			continue;
		}

		// sigh... transform the bounds to where they should be for the given face
		PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( brush ) );
		bounds              = brush->faces[ i ].bounds;
		bounds.origin       = PlGetMatrix4Translation( &transform );

		// check that the bounds are in view
		if ( !PlgIsBoxInsideView( self->internal, &bounds ) )
		{
			continue;
		}

		// ensure it's facing the camera
		PLVector3 faceOrigin     = PlAddVector3( bounds.absOrigin, bounds.origin );
		PLVector3 cameraPosition = ape_camera_get_position( self );
		PLVector3 view           = PlNormalizeVector3( PlSubtractVector3( cameraPosition, faceOrigin ) );
		if ( PlVector3DotProduct( brush->faces[ i ].normal, view ) < 0.0f )
		{
			continue;
		}

		if ( ape_brush_face_is_portal( &brush->faces[ i ] ) )
		{
			// clamp it to the *absolute* maximum depth;
			// this setting is here for performance reasons,
			// so users can toggle it for their system, but not to
			// rediculous degrees...
			PL_GET_CVAR( "renderer.maxPortalDepth", maxPortalDepth );
			if ( maxPortalDepth->i_value > APE_CAMERA_MAX_PORTAL_DEPTH )
			{
				char tmp[ 64 ];
				snprintf( tmp, sizeof( tmp ), "%u", APE_CAMERA_MAX_PORTAL_DEPTH );
				PlSetConsoleVariable( maxPortalDepth, tmp );
			}

			if ( visibleRoom->numPortals < APE_CAMERA_MAX_ROOM_PORTALS && ape_rendererState_.depth < maxPortalDepth->i_value )
			{
				ApeCameraVisiblePortal *visiblePortal = &visibleRoom->portals[ visibleRoom->numPortals ];
				if ( visibleRoom->entrance != nullptr && visibleRoom->entrance->portalFace == &brush->faces[ i ] )
				{
					// okay, this is probably the same face we're looking in from? So skip it.
					continue;
				}

				PLVector4 screenRect = get_face_screen_rect( &brush->faces[ i ], self, viewport );
				if ( screenRect.z == 0.f || screenRect.w == 0.f )
				{
					continue;
				}

				ApeBrushFace *destinationFace = ape_brush_face_get_portal_destination( &brush->faces[ i ] );
				if ( destinationFace == nullptr )
				{
					continue;
				}

				visiblePortal->screenRect = screenRect;
				visiblePortal->portalFace = &brush->faces[ i ];
				visiblePortal->origin     = faceOrigin;

				visiblePortal->normal = brush->faces[ i ].normal;
				PLVector4 tmp         = PlTransformVector4( &PL_VEC3TO4( visiblePortal->normal ), &transform );
				visiblePortal->normal = PlNormalizeVector3( PL_VEC4TO3( tmp ) );

				visibleRoom->numPortals++;

				if ( self->pvs.numRooms < APE_CAMERA_MAX_ROOMS )
				{
					visiblePortal->nextRoom = &self->pvs.rooms[ self->pvs.numRooms ];
					self->pvs.numRooms++;

					// update the view matrix
					visiblePortal->nextRoom->viewMatrix = self->internal->internal.view;
					if ( ape_brush_face_is_mirror( &brush->faces[ i ] ) )
					{
						PLMatrix4 reflection;
						setup_reflection_matrix( &visiblePortal->normal, &visiblePortal->origin, &reflection );
						visiblePortal->nextRoom->viewMatrix = PlMultiplyMatrix4( &visiblePortal->nextRoom->viewMatrix, &reflection );
					}
					else
					{
						//TODO: handle the non mirror case

						ApeBrush *destinationBrush     = destinationFace->parent;
						PLMatrix4 destinationTransform = ape_world_node_get_transform( APE_WORLD_NODE( brush ) );

						visiblePortal->nextRoom->viewMatrix = visibleRoom->viewMatrix;
					}

					visiblePortal->nextRoom->room = ape_brush_face_get_room( destinationFace );

					self->internal->internal.view = visiblePortal->nextRoom->viewMatrix;
					PlgSetViewMatrix( &visiblePortal->nextRoom->viewMatrix );
					PlgSetupCameraFrustum( self->internal );

					ape_rendererState_.depth++;

					visiblePortal->nextRoom->entrance = visiblePortal;
					pvs_navigate_node_tree( self, viewport, APE_WORLD_NODE( visiblePortal->nextRoom->room ), visiblePortal->nextRoom );

					ape_rendererState_.depth--;

					self->internal->internal.view = visibleRoom->viewMatrix;
					PlgSetViewMatrix( &visibleRoom->viewMatrix );
					PlgSetupCameraFrustum( self->internal );
				}
				else
				{
					ape_warning_( "Hit max visible room limit (%u >= %u)!\n", self->pvs.numRooms, APE_CAMERA_MAX_ROOMS );
				}
			}
		}

		numVisibleFaces++;
	}

	if ( numVisibleFaces == 0 )
	{
		return false;
	}

	return true;
}

static void pvs_navigate_node_tree( ApeCamera *self, const ApeViewport *viewport, ApeWorldNode *worldNode, ApeCameraVisibleRoom *visibleRoom )
{
	// show the node volumes, but mind this is only active for the *top* room!
	if ( ape_config_.world.showNodeVolumes && ( &self->pvs.rooms[ 0 ] == visibleRoom ) )
	{
		PLCollisionAABB transformedBounds = ape_world_node_get_transformed_local_bounds( worldNode );
		PLMatrix4       transform         = ape_world_node_get_transform( worldNode );
		PLVector3       pos               = PlGetMatrix4Translation( &transform );
		ape_draw_debug_axis( pos, worldNode->angles, 16.0f );
		ape_draw_debug_aabb( &worldNode->bounds, PL_COLOUR_PURPLE );
		ape_draw_debug_aabb( &transformedBounds, PL_COLOUR_ORANGE );
	}

	if ( worldNode->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		if ( visibleRoom->numLights < APE_CAMERA_MAX_ROOM_LIGHTS )
		{
			ApeLight *light = ( ApeLight * ) worldNode;
			if ( pvs_test_light( self, light ) )
			{
				visibleRoom->lights[ visibleRoom->numLights ] = light;
				visibleRoom->numLights++;

				ape_rendererPerformance_.numLights++;
			}
		}
		else
		{
			ape_warning_( "Hit max visible light limit (%u >= %u)!\n", visibleRoom->numLights, APE_CAMERA_MAX_ROOM_LIGHTS );
		}
	}
	else
	{
		if ( visibleRoom->numNodes < APE_CAMERA_MAX_ROOM_NODES )
		{
			bool isVisible;
			if ( worldNode->type == APE_WORLD_NODE_TYPE_BRUSH )
			{
				isVisible = pvs_test_brush( self, viewport, ( ApeBrush * ) worldNode, visibleRoom );
			}
			else
			{
				PLCollisionAABB transformedBounds = ape_world_node_get_transformed_local_bounds( worldNode );
				isVisible                         = PlgIsBoxInsideView( self->internal, &transformedBounds );
			}

			if ( isVisible )
			{
				visibleRoom->nodes[ visibleRoom->numNodes ] = worldNode;
				visibleRoom->numNodes++;
			}
		}
		else
		{
			ape_warning_( "Hit max visible node limit (%u >= %u)!\n", visibleRoom->numNodes, APE_CAMERA_MAX_ROOM_NODES );
		}
	}

	ApeWorldNode *childNode;
	COM_ITERATE_LINKED_LIST( childNode, worldNode->children, i )
	{
		pvs_navigate_node_tree( self, viewport, childNode, visibleRoom );
	}
}

void ape_camera_build_pvs_( ApeCamera *self, const ApeViewport *viewport )
{
	self->pvs = ( ApeCameraVisibleSet ) {};

	ApeRoom *room = ape_camera_get_room( self );
	if ( room == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	self->pvs.rooms[ 0 ].room       = room;
	self->pvs.rooms[ 0 ].viewMatrix = self->internal->internal.view;
	self->pvs.numRooms++;

	pvs_navigate_node_tree( self, viewport, APE_WORLD_NODE( room ), &self->pvs.rooms[ 0 ] );

	ape_rendererPerformance_.numRooms = self->pvs.numRooms;

	COM_PROFILE_FUNCTION_END();
}
