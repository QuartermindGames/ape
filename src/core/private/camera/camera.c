// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Core camera implementation.

#include <plcore/pl_hashtable.h>

#include "ape_private.h"

#include "camera.h"

#include "renderer/renderer.h"
#include "renderer/post/post.h"
#include "world/world.h"

static constexpr float CAMERA_DEFAULT_FOCUS_POINT = 16.0f;
static constexpr float CAMERA_DEFAULT_FOCUS_SCALE = 0.f;
static constexpr float CAMERA_DEFAULT_APERTURE    = 0.f;

static constexpr float CAMERA_DEFAULT_FOV  = 75.0f;
static constexpr float CAMERA_DEFAULT_NEAR = 0.1f;
static constexpr float CAMERA_DEFAULT_FAR  = 1000.0f;

void ape_camera_make_active( ApeCamera *camera )
{
	if ( camera != nullptr )
	{
		ape_camera_setup( camera );
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

	if ( camera->mode == APE_CAMERA_MODE_PERSPECTIVE || camera->mode == APE_CAMERA_MODE_ISOMETRIC )
	{
		camera->fov  = CAMERA_DEFAULT_FOV;
		camera->far  = CAMERA_DEFAULT_FAR;
		camera->near = CAMERA_DEFAULT_NEAR;
	}
	else
	{
		camera->near = -10000.0f;
		camera->far  = 10000.0f;
	}
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

static void *camera_create( ApeWorldNode *parent )
{
	ApeCamera *camera = QM_OS_MEMORY_NEW( ApeCamera );
	ape_world_node_setup_( &camera->base, parent, APE_WORLD_NODE_TYPE_CAMERA, nullptr, &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO );

	camera->dof.focusPoint = CAMERA_DEFAULT_FOCUS_POINT;
	camera->dof.focusScale = CAMERA_DEFAULT_FOCUS_SCALE;
	camera->dof.aperture   = CAMERA_DEFAULT_APERTURE;

	ape_camera_set_view_mode( camera, APE_CAMERA_MODE_PERSPECTIVE );
	ape_camera_set_draw_mode( camera, APE_CAMERA_DRAW_MODE_SHADED );

	return camera;
}

ApeCamera *ape_create_camera( ApeWorldNode *parent, const char *name, const QmMathVector3f *position, const QmMathVector3f *angles, ApeCameraViewMode cameraMode, ApeCameraDrawMode drawMode )
{
	ApeCamera *camera = camera_create( parent );

	ape_world_node_set_name( APE_WORLD_NODE( camera ), name );

	ape_camera_set_view_mode( camera, cameraMode );
	ape_camera_set_draw_mode( camera, drawMode );

	ape_camera_set_position( camera, position );
	ape_camera_set_angles( camera, angles );

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

	qm_os_memory_free( self );
}

void ape_camera_set_position( ApeCamera *self, const QmMathVector3f *position )
{
	ape_world_node_set_position( &self->base, position );
}

void ape_camera_set_angles( ApeCamera *camera, const QmMathVector3f *angles )
{
	ape_world_node_set_angles( &camera->base, angles );
}

QmMathVector3f ape_camera_get_position( const ApeCamera *camera )
{
	return ape_world_node_get_local_position( &camera->base );
}

QmMathVector3f ape_camera_get_angles( const ApeCamera *camera )
{
	return ape_world_node_get_angles( &camera->base );
}

QmMathVector3f ape_camera_get_forward( const ApeCamera *camera )
{
	PLMatrix4 view = camera->view;
	return qm_math_vector3f( view.mm[ 0 ][ 2 ], view.mm[ 1 ][ 2 ], view.mm[ 2 ][ 2 ] );
}

void ape_camera_setup_frustum( ApeCamera *camera )
{
	PLMatrix4 viewProj = PlMultiplyMatrix4( &camera->proj, &camera->view );

	// Right
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_RIGHT ].x = viewProj.m[ 3 ] - viewProj.m[ 0 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_RIGHT ].y = viewProj.m[ 7 ] - viewProj.m[ 4 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_RIGHT ].z = viewProj.m[ 11 ] - viewProj.m[ 8 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_RIGHT ].w = viewProj.m[ 15 ] - viewProj.m[ 12 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_RIGHT ]   = PlNormalizePlane( camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_RIGHT ] );
	// Left
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_LEFT ].x = viewProj.m[ 3 ] + viewProj.m[ 0 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_LEFT ].y = viewProj.m[ 7 ] + viewProj.m[ 4 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_LEFT ].z = viewProj.m[ 11 ] + viewProj.m[ 8 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_LEFT ].w = viewProj.m[ 15 ] + viewProj.m[ 12 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_LEFT ]   = PlNormalizePlane( camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_LEFT ] );
	// Bottom
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_BOTTOM ].x = viewProj.m[ 3 ] - viewProj.m[ 1 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_BOTTOM ].y = viewProj.m[ 7 ] - viewProj.m[ 5 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_BOTTOM ].z = viewProj.m[ 11 ] - viewProj.m[ 9 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_BOTTOM ].w = viewProj.m[ 15 ] - viewProj.m[ 13 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_BOTTOM ]   = PlNormalizePlane( camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_BOTTOM ] );
	// Top
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_TOP ].x = viewProj.m[ 3 ] + viewProj.m[ 1 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_TOP ].y = viewProj.m[ 7 ] + viewProj.m[ 5 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_TOP ].z = viewProj.m[ 11 ] + viewProj.m[ 9 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_TOP ].w = viewProj.m[ 15 ] + viewProj.m[ 13 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_TOP ]   = PlNormalizePlane( camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_TOP ] );
	// Far
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_FAR ].x = viewProj.m[ 3 ] - viewProj.m[ 2 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_FAR ].y = viewProj.m[ 7 ] - viewProj.m[ 6 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_FAR ].z = viewProj.m[ 11 ] - viewProj.m[ 10 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_FAR ].w = viewProj.m[ 15 ] - viewProj.m[ 14 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_FAR ]   = PlNormalizePlane( camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_FAR ] );
	// Near
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_NEAR ].x = viewProj.m[ 3 ] + viewProj.m[ 2 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_NEAR ].y = viewProj.m[ 7 ] + viewProj.m[ 6 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_NEAR ].z = viewProj.m[ 11 ] + viewProj.m[ 10 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_NEAR ].w = viewProj.m[ 15 ] + viewProj.m[ 14 ];
	camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_NEAR ]   = PlNormalizePlane( camera->frustum[ APE_CAMERA_FRUSTUM_PLANE_NEAR ] );
}

void ape_camera_setup( ApeCamera *camera )
{
	int w, h;
	qm_gfx_get_viewport( nullptr, nullptr, &w, &h );

	switch ( camera->mode )
	{
		case APE_CAMERA_MODE_PERSPECTIVE:
		{
			camera->proj = PlPerspective( camera->fov, ( float ) w / ( float ) h, camera->near, camera->far );
			break;
		}
		case APE_CAMERA_MODE_ORTHOGRAPHIC:
		{
			camera->proj = PlOrtho( 0, ( float ) w, ( float ) h, 0, camera->near, camera->far );
			camera->view = PlMatrix4Identity();
			break;
		}
		case APE_CAMERA_MODE_ISOMETRIC:
		{
			camera->proj = PlOrtho( -camera->fov, camera->fov, -camera->fov, 5, -5, 40 );
			camera->view = PlMatrix4Identity();
			break;
		}
		default:
			break;
	}

	if ( camera->mode != APE_CAMERA_MODE_ORTHOGRAPHIC )
	{
		PlMatrixMode( PL_VIEW_MATRIX );
		PlLoadIdentityMatrix();

		QmMathVector3f angles = ape_camera_get_angles( camera );
		PlRotateMatrix3f( QM_MATH_DEG2RAD( -angles.x ), 1.0f, 0.0f, 0.0f );
		PlRotateMatrix3f( QM_MATH_DEG2RAD( -angles.y ), 0.0f, 1.0f, 0.0f );
		PlRotateMatrix3f( QM_MATH_DEG2RAD( -angles.z ), 0.0f, 0.0f, 1.0f );

		QmMathVector3f position = ape_camera_get_position( camera );
		PlTranslateMatrix( QM_MATH_VECTOR3F( -position.x, -position.y, -position.z ) );

		camera->view = *PlGetMatrix( PL_VIEW_MATRIX );
	}

	// setup the camera frustum
	ape_camera_setup_frustum( camera );

	// copy camera matrices
	PlgSetViewMatrix( &camera->view );
	PlgSetProjectionMatrix( &camera->proj );
}

void ape_draw_scene_( ApeCamera *camera, const ApeViewport *viewport );
void ape_camera_draw_perspective( ApeCamera *camera, const ApeViewport *viewport )
{
	assert( camera != nullptr && viewport != nullptr );

	COM_PROFILE_FUNCTION_START();

#if 0
	//TODO: ditch this mechanism, probably!
	if ( camera->mode == APE_CAMERA_MODE_ISOMETRIC )
	{
		// Uh, let's hardcode it for this as I can't think why you would want anything else -
		// this is what the other modes are there for!
		camera->internal->angles.x = -35.264f;
	}
#endif

	ape_camera_setup( camera );

	// Draw the scene into a buffer
	ape_draw_scene_( camera, viewport );

	// now apply post processing to the buffer
	ape_postfx_draw_( viewport, camera );

	COM_PROFILE_FUNCTION_END();
}

void ape_camera_set_fov( ApeCamera *camera, float fov )
{
	if ( fov < 1.0f )
	{
		fov = 1.0f;
	}
	else if ( fov > 179.0f )
	{
		fov = 179.0f;
	}

	camera->fov = fov;
}

float ape_camera_get_fov( const ApeCamera *camera )
{
	return camera->fov;
}

void ape_camera_set_focus_point( ApeCamera *self, const float focusPoint )
{
	self->dof.focusPoint = focusPoint;
}

void ape_camera_set_focus_scale( ApeCamera *self, const float focusScale )
{
	self->dof.focusScale = focusScale;
}

void ape_camera_set_aperture( ApeCamera *self, const float aperture )
{
	self->dof.aperture = aperture;
}

/**
 * Checks that the given bounding box is within the view space.
 */
bool ape_camera_test_box( const ApeCamera *camera, const PLCollisionAABB *bounds )
{
	QmMathVector3f mins = qm_math_vector3f_add( bounds->mins, bounds->origin );
	QmMathVector3f maxs = qm_math_vector3f_add( bounds->maxs, bounds->origin );
	for ( unsigned int i = 0; i < APE_CAMERA_MAX_FRUSTUM_PLANES; ++i )
	{
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &QM_MATH_VECTOR3F( mins.x, mins.y, mins.z ) ) >= 0.0f )
		{
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &QM_MATH_VECTOR3F( maxs.x, mins.y, mins.z ) ) >= 0.0f )
		{
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &QM_MATH_VECTOR3F( mins.x, maxs.y, mins.z ) ) >= 0.0f )
		{
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &QM_MATH_VECTOR3F( maxs.x, maxs.y, mins.z ) ) >= 0.0f )
		{
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &QM_MATH_VECTOR3F( mins.x, mins.y, maxs.z ) ) >= 0.0f )
		{
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &QM_MATH_VECTOR3F( mins.x, maxs.y, maxs.z ) ) >= 0.0f )
		{
			continue;
		}
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &QM_MATH_VECTOR3F( maxs.x, maxs.y, maxs.z ) ) >= 0.0f )
		{
			continue;
		}

		return false;
	}

	return true;
}

/**
 * Checks that the given sphere is within the view space.
 */
bool ape_camera_test_sphere( const ApeCamera *camera, const PLCollisionSphere *sphere )
{
	for ( unsigned int i = 0; i < APE_CAMERA_MAX_FRUSTUM_PLANES; ++i )
	{
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &sphere->origin ) < -sphere->radius )
		{
			return false;
		}
	}

	return true;
}

bool ape_camera_test_point( const ApeCamera *camera, const QmMathVector3f point )
{
	for ( unsigned int i = 0; i < APE_CAMERA_MAX_FRUSTUM_PLANES; ++i )
	{
		if ( PlGetPlaneDotProduct( &camera->frustum[ i ], &point ) < 0.0f )
		{
			return false;
		}
	}

	return true;
}

static ApeWorldNode *camera_clone( ApeWorldNode *src )
{
	ApeCamera *srcCamera = ( ApeCamera * ) src;
	ApeCamera *dstCamera = ape_create_camera( src->parent, src->name, &src->position, &src->angles, srcCamera->mode, srcCamera->drawMode );
	if ( dstCamera == nullptr )
	{
		ape_console_warning_( "Failed to create camera for duplication!\n" );
		return nullptr;
	}

	dstCamera->active = srcCamera->active;
	dstCamera->dof    = srcCamera->dof;

	return APE_WORLD_NODE( dstCamera );
}

static AcmBranch *camera_serialize( void *self, AcmBranch *root )
{
	ApeCamera *camera = self;
	acm_push_bool( root, "active", camera->active );
	acm_push_f32( root, "aperture", camera->dof.aperture );
	acm_push_f32( root, "focusPoint", camera->dof.focusPoint );
	acm_push_f32( root, "focusScale", camera->dof.focusScale );
	acm_push_i8( root, "drawMode", camera->drawMode );
	acm_push_i8( root, "mode", camera->mode );

	return root;
}

static ApeWorldNode *camera_deserialize( ApeWorldNode *self, AcmBranch *root )
{
	ApeCamera *camera      = ( ApeCamera * ) self;
	camera->active         = acm_get_bool( root, "active", camera->active );
	camera->dof.aperture   = acm_get_f32( root, "aperture", camera->dof.aperture );
	camera->dof.focusPoint = acm_get_f32( root, "focusPoint", camera->dof.focusPoint );
	camera->dof.focusScale = acm_get_f32( root, "focusScale", camera->dof.focusScale );
	camera->drawMode       = acm_get_int( root, "drawMode", camera->drawMode );
	camera->mode           = acm_get_int( root, "mode", camera->mode );

	return self;
}

const ApeWorldNodeClass ape_cameraClass = {
        .identifier = "camera",
        .magic      = QM_OS_MAGIC_TO_NUM( 'C', 'A', 'M', ' ' ),

        .create  = camera_create,
        .destroy = ape_camera_destroy_,

        .serialize   = camera_serialize,
        .deserialize = camera_deserialize,

        .clone = camera_clone,

        .editorIcon = "resources/new_camera.gif",
};

/////////////////////////////////////////////////////////////////////////////////////
// PVS Generation

static void pvs_navigate_node_tree( ApeCamera *self, const ApeViewport *viewport, ApeWorldNode *worldNode, ApeCameraVisibleRoom *visibleRoom );

static QmMathVector4f get_face_screen_rect( const ApeBrushFace *face, const ApeCamera *camera, const ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	PLMatrix4 viewProj = PlMultiplyMatrix4( &camera->proj, &camera->view );

	QmMathVector4f rect = qm_math_vector4f( viewport->width, viewport->height, 0.0f, 0.0f );

	// get the transform
	ApeBrush *brush = face->parent;
	assert( brush != nullptr );
	PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( brush ) );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		assert( face->vertices[ i ].posIndex < face->parent->numVertices );
		QmMathVector3f vertex = PlTransformVector3( &face->parent->vertices[ face->vertices[ i ].posIndex ], &transform );

		float          depth;
		QmMathVector2f screenPos = PlConvertWorldToScreen( &vertex, &viewProj, ( int[] ) { 0, 0, viewport->width, viewport->height }, &depth, true );

		if ( screenPos.x < rect.x )
		{
			rect.x = QM_MATH_CLAMP( 0.0f, screenPos.x, viewport->width );
		}
		if ( screenPos.x > rect.z )
		{
			rect.z = QM_MATH_CLAMP( 0.0f, screenPos.x, viewport->width );
		}

		// sigh... we need to flip it, again
		screenPos.y = viewport->height - screenPos.y;
		if ( screenPos.y < rect.y )
		{
			rect.y = QM_MATH_CLAMP( 0.0f, screenPos.y, viewport->height );
		}
		if ( screenPos.y > rect.w )
		{
			rect.w = QM_MATH_CLAMP( 0.0f, screenPos.y, viewport->height );
		}
	}

	rect.z -= rect.x;
	rect.w -= rect.y;

	COM_PROFILE_FUNCTION_END();

	return rect;
}

static bool pvs_test_light( ApeCamera *self, ApeLight *light )
{
	QmMathVector3f position = ape_light_get_position( light );
	if ( light->type != APE_LIGHT_TYPE_SUN )
	{
		float distance = qm_math_vector3f_length( qm_math_vector3f_sub( position, ape_camera_get_position( self ) ) );
		if ( distance > ape_config_.renderer.maxLightDistance )
		{
			return false;
		}

		if ( !ape_camera_test_sphere( self, &PlSetupCollisionSphere( position, light->radius <= 0.0f ? 1.0f : light->radius ) ) )
		{
			return false;
		}
	}

	if ( ape_config_.renderer.showLights )
	{
		ApeRoom *room = ape_camera_get_room( self );
		if ( room == ape_world_node_get_room( APE_WORLD_NODE( light ) ) )
		{
			ape_draw_debug_sphere( position, QM_MATH_COLOUR4F_TO_4UB( light->colour ), light->radius );
			if ( light->type != APE_LIGHT_TYPE_OMNI )
			{
				QmMathVector3f angles = ape_world_node_get_angles( APE_WORLD_NODE( light ) );
				QmMathVector3f forward;
				PlAnglesAxes( angles, nullptr, nullptr, &forward );
				QmMathVector3f end = qm_math_vector3f_add( position, qm_math_vector3f_scale_float( forward, 16.0f ) );
				ape_draw_debug_arrow( position, end, QM_MATH_COLOUR4F_TO_4UB( light->colour ), 1.0f );
			}
		}
	}

	// for now, for simplicity-sake, flares only work so long as the camera is in the same room
	ApeRoom *room = ape_camera_get_room( self );
	if ( light->flags & APE_LIGHT_FLAG_FLARE && room == ape_world_node_get_room( APE_WORLD_NODE( light ) ) )
	{
		//TODO: test the flare is actually visible!!
		ape_add_flare_to_queue( self, &position, &QM_MATH_COLOUR4F_RGB( light->colour.r, light->colour.g, light->colour.b ), 1.0f, light->colour.a );
	}

	return true;
}

static void setup_reflection_matrix( const QmMathVector3f *normal, const QmMathVector3f *planePoint, PLMatrix4 *reflectionMatrix )
{
	const float d = -qm_math_vector3f_dot_product( *normal, *planePoint );

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
	if ( !ape_camera_test_box( self, &bounds ) )
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
		if ( !ape_camera_test_box( self, &bounds ) )
		{
			continue;
		}

		// ensure it's facing the camera
		QmMathVector3f faceOrigin     = qm_math_vector3f_add( bounds.absOrigin, bounds.origin );
		QmMathVector3f cameraPosition = ape_camera_get_position( self );
		QmMathVector3f view           = qm_math_vector3f_normalize( qm_math_vector3f_sub( cameraPosition, faceOrigin ) );
		if ( qm_math_vector3f_dot_product( brush->faces[ i ].normal, view ) < 0.0f )
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

				QmMathVector4f screenRect = get_face_screen_rect( &brush->faces[ i ], self, viewport );
				if ( screenRect.z == 0.f || screenRect.w == 0.f )
				{
					continue;
				}

				ApeBrushFace *destinationFace = ape_brush_face_get_portal_destination( &brush->faces[ i ] );
				if ( destinationFace == nullptr )
				{
					numVisibleFaces++;
					continue;
				}

				visiblePortal->screenRect = screenRect;
				visiblePortal->portalFace = &brush->faces[ i ];
				visiblePortal->origin     = faceOrigin;

				visiblePortal->normal = brush->faces[ i ].normal;
				QmMathVector4f tmp    = PlTransformVector4( &QM_MATH_VECTOR4F( visiblePortal->normal.x,
				                                                               visiblePortal->normal.y,
				                                                               visiblePortal->normal.z, 0.0f ),
				                                            &transform );
				visiblePortal->normal = qm_math_vector3f_normalize( QM_MATH_VECTOR3F( tmp.x, tmp.y, tmp.z ) );

				visibleRoom->numPortals++;

				if ( self->pvs.numRooms < APE_CAMERA_MAX_ROOMS )
				{
					visiblePortal->nextRoom = &self->pvs.rooms[ self->pvs.numRooms ];
					self->pvs.numRooms++;

					// update the view matrix
					visiblePortal->nextRoom->viewMatrix = self->view;
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

					self->view = visiblePortal->nextRoom->viewMatrix;
					PlgSetViewMatrix( &visiblePortal->nextRoom->viewMatrix );
					ape_camera_setup_frustum( self );

					ape_rendererState_.depth++;

					visiblePortal->nextRoom->entrance = visiblePortal;
					pvs_navigate_node_tree( self, viewport, APE_WORLD_NODE( visiblePortal->nextRoom->room ), visiblePortal->nextRoom );

					ape_rendererState_.depth--;

					self->view = visibleRoom->viewMatrix;
					PlgSetViewMatrix( &visibleRoom->viewMatrix );
					ape_camera_setup_frustum( self );
				}
				else
				{
					ape_console_warning_( "Hit max visible room limit (%u >= %u)!\n", self->pvs.numRooms, APE_CAMERA_MAX_ROOMS );
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
		QmMathVector3f  pos               = ape_world_node_get_position( worldNode );
		QmMathVector3f  ang               = ape_world_node_get_angles( worldNode );
		ape_draw_debug_axis( pos, ang, 16.0f );
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
			ape_console_warning_( "Hit max visible light limit (%u >= %u)!\n", visibleRoom->numLights, APE_CAMERA_MAX_ROOM_LIGHTS );
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
				isVisible                         = ape_camera_test_box( self, &transformedBounds );
			}

			if ( isVisible )
			{
				visibleRoom->nodes[ visibleRoom->numNodes ] = worldNode;
				visibleRoom->numNodes++;
			}
		}
		else
		{
			ape_console_warning_( "Hit max visible node limit (%u >= %u)!\n", visibleRoom->numNodes, APE_CAMERA_MAX_ROOM_NODES );
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
	self->pvs.rooms[ 0 ].viewMatrix = self->view;
	self->pvs.numRooms++;

	pvs_navigate_node_tree( self, viewport, APE_WORLD_NODE( room ), &self->pvs.rooms[ 0 ] );

	ape_rendererPerformance_.numRooms = self->pvs.numRooms;

	COM_PROFILE_FUNCTION_END();
}
