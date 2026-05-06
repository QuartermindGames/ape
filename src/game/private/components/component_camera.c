// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Camera manager/helpers for game.
// Author:  Mark E. Sowden

#include "game_private.h"
#include "component_camera.h"

static constexpr float CAMERA_DEFAULT_HEIGHT   = 45.0f;
static constexpr float CAMERA_DEFAULT_DISTANCE = 50.0f;
static constexpr float CAMERA_DEFAULT_SIDE     = 10.0f;

//TODO: this shouldn't be here...
static void free_camera_tick( const GameCameraComponent *component, ApeCamera *camera, const double delta )
{
	QmMathVector3f ang = ape_camera_get_angles( camera );
	QmMathVector3f pos = ape_camera_get_position( camera );

	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
	{
		int mx, my;
		ape_client_input_get_mouse_delta( &mx, &my );
		ang.y += ( float ) mx;
		ang.x += ( float ) my;
		ang.x = QM_MATH_CLAMP( -90.0f, ang.x, 90.0f );
	}

	QmMathVector2f rightStick = ape_client_input_get_controller_axis_state( 0, 1 );
	ang.x -= rightStick.y * 100.0f * delta;
	ang.y -= rightStick.x * 150.0f * delta;

	QmMathVector3f forward, left;
	PlAnglesAxes( ang, &left, nullptr, &forward );

	QmMathVector2f leftStick = ape_client_input_get_controller_axis_state( 0, 0 );
	pos                      = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( forward, leftStick.y * 100.0f * delta ) );
	pos                      = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( left, leftStick.x * 100.0f * delta ) );

	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( camera ) );
	if ( room != nullptr )
	{
		PLCollisionSphere sphere = {};
		sphere.origin            = pos;
		sphere.radius            = 4.0f;

		ApeCollisionCollider collider = {};
		collider.type                 = APE_COLLISION_TYPE_SPHERE;
		collider.sphere               = &sphere;

		unsigned int              numHits;
		ApeCollisionIntersection *hits;
		if ( ( hits = ape_room_intersect( room, &collider, &numHits ) ) != nullptr )
		{
			for ( unsigned int i = 0; i < numHits; ++i )
			{
				if ( hits[ i ].face != nullptr )
				{
					PLCollisionPlane plane = {};
					plane.origin           = hits[ i ].face->bounds.absOrigin;
					plane.normal           = hits[ i ].face->normal;
					ape_draw_debug_plane( &plane, PL_COLOUR_RED, 32.0f );

					float penetrationDepth = sphere.radius - hits[ i ].distance;
					if ( penetrationDepth > 0.0f )
					{
						QmMathVector3f collisionDirection = qm_math_vector3f_normalize( qm_math_vector3f_sub( sphere.origin, hits[ i ].intersection ) );
						pos                               = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( collisionDirection, penetrationDepth ) );
					}
				}

				ape_draw_debug_axis( hits[ i ].intersection, QM_MATH_VECTOR3F_ZERO, 2.0f );
			}

			qm_os_memory_free( hits );
		}
	}

	ape_camera_set_position( camera, &pos );
	ape_camera_set_angles( camera, &ang );
}

static void third_person_tick( const GameCameraComponent *component, ApeCamera *camera, const double delta )
{
	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( camera ) );
	if ( parent == nullptr )
	{
		return;
	}

	QmMathVector3f cpos = ape_camera_get_position( camera );
	QmMathVector3f cang = ape_camera_get_angles( camera );

	// entity camera position + view height
	QmMathVector3f epos = ape_world_node_get_position( parent );
	epos.y              = epos.y + component->height;
	// entity camera angles
	QmMathVector3f eang = component->angles;

	QmMathVector3f forward, left;
	PlAnglesAxes( eang, &left, nullptr, &forward );

	// push entity position out and to either side
	QmMathVector3f npos = epos;
	npos                = qm_math_vector3f_add( npos, qm_math_vector3f_scale_float( forward, component->distance ) );
	npos                = qm_math_vector3f_add( npos, qm_math_vector3f_scale_float( left, component->side ) );

	// now interpolate the position and angles for the camera to the new position
	cpos = PlLinearInterpolateV3f( cpos, npos, 7.0f * delta );
	com_math_interpolate_angles( &cang, &eang, 7.0f * delta, &cang );

	// if the camera is hitting anything, move it
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( camera ) );
	if ( room != nullptr )
	{
		PLCollisionRay ray = {};
		ray.origin         = epos;
		ray.direction      = qm_math_vector3f_sub( npos, epos );

		ApeCollisionIntersection result = {};
		if ( ape_room_ray_intersect( room, &ray, &result ) && result.distance <= component->distance )
		{
			cpos = result.intersection;
		}
	}

	ape_camera_set_position( camera, &cpos );
	ape_camera_set_angles( camera, &cang );
}

static void first_person_tick( const GameCameraComponent *component, ApeCamera *camera, const double delta )
{
	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( camera ) );
	if ( parent == nullptr )
	{
		return;
	}

	QmMathVector3f cpos = ape_camera_get_position( camera );
	QmMathVector3f cang = ape_camera_get_angles( camera );

	// entity camera position + view height
	QmMathVector3f epos = ape_world_node_get_position( parent );
	epos.y              = epos.y + component->height;

	// entity camera angles
	QmMathVector3f eang = component->angles;

	QmMathVector3f forward, left;
	PlAnglesAxes( eang, &left, nullptr, &forward );

	// now interpolate the position and angles for the camera to the new position
	cpos = PlLinearInterpolateV3f( cpos, epos, 7.0f * delta );
	com_math_interpolate_angles( &cang, &eang, 16.0f * delta, &cang );

	ape_camera_set_position( camera, &cpos );
	ape_camera_set_angles( camera, &cang );
}

static void *component_camera_create()
{
	GameCameraComponent *component = QM_OS_MEMORY_NEW( GameCameraComponent );
	component->distance            = CAMERA_DEFAULT_DISTANCE;
	component->side                = CAMERA_DEFAULT_SIDE;
	component->height              = CAMERA_DEFAULT_HEIGHT;

	return component;
}

static void component_camera_destroy( void *data )
{
}

static AcmBranch *component_camera_serialize( void *ptr, AcmBranch *root )
{
	GameCameraComponent *component = ptr;
	acm_push_f32( root, "distance", component->distance );
	acm_push_f32( root, "side", component->side );
	acm_push_f32( root, "height", component->height );
	com_acm_push_vector3( root, "angles", &component->angles, true );

	return root;
}

static void *component_camera_deserialize( void *ptr, AcmBranch *root )
{
	GameCameraComponent *component = ptr;
	component->distance            = acm_get_f32( root, "distance", component->distance );
	component->side                = acm_get_f32( root, "side", component->side );
	component->height              = acm_get_f32( root, "height", component->height );
	component->angles              = com_acm_get_vector3( root, "angles", &QM_MATH_VECTOR3F_ZERO );

	return component;
}

void game_component_camera_handle_input_( GameCameraComponent *component, double delta )
{
	QmMathVector2f rightStick = ape_client_input_get_controller_axis_state( 0, 1 );

	// update the pitch
	component->angles.x -= rightStick.y * 100.0f * delta;
	if ( component->angles.x > 90.0f )
	{
		component->angles.x = 90.0f;
	}
	else if ( component->angles.x < -90.0f )
	{
		component->angles.x = -90.0f;
	}

	// and the yaw
	component->angles.y -= rightStick.x * 150.0f * delta;

	com_math_normalize_angles( &component->angles, &component->angles );
}

void game_component_camera_tick_( const GameCameraComponent *component, ApeCamera *camera, const double delta )
{
	// wha?
	//if ( component->state == component->oldState )
	//{
	//	// probably not transitioning between states...
	//	return;
	//}

	if ( component->state == GAME_CAMERA_STATE_FREE )
	{
		free_camera_tick( component, camera, delta );
		return;
	}

	if ( component->state == GAME_CAMERA_STATE_THIRD_PERSON )
	{
		third_person_tick( component, camera, delta );
	}
	else
	{
		first_person_tick( component, camera, delta );
	}
}

void game_component_camera_cycle_state_( GameCameraComponent *component )
{
	component->oldState = component->state;

	component->state++;
	if ( component->state >= GAME_CAMERA_STATE_MAX )
	{
		component->state = 0;
	}

	switch ( component->state )
	{
		default:
		case GAME_CAMERA_STATE_FREE:
			game_print_( "Free Camera\n" );
			break;
		case GAME_CAMERA_STATE_FIRST_PERSON:
			game_print_( "First-Person Camera\n" );
			break;
		case GAME_CAMERA_STATE_THIRD_PERSON:
			game_print_( "Third-Person Camera\n" );
			break;
	}
}

ApeEntityComponentDefinition game_cameraComponent_ = {
        .name                = GAME_CAMERA_COMPONENT_NAME,
        .createFunction      = component_camera_create,
        .destroyFunction     = component_camera_destroy,
        .serializeFunction   = component_camera_serialize,
        .deserializeFunction = component_camera_deserialize,
};
