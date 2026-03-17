// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Lights
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "renderer/renderer.h"
#include "renderer/material/material.h"

#include "world/world.h"

static void *create_light( ApeWorldNode *parent )
{
	ApeLight *light = QM_OS_MEMORY_NEW( ApeLight );
	ape_world_node_setup_( &light->base, parent, APE_WORLD_NODE_TYPE_LIGHT, nullptr, &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO );

	light->colour = QM_MATH_COLOUR4F( 1.0f, 1.0f, 1.0f, 1.0f );

	return light;
}

ApeLight *ape_create_light( ApeWorldNode *parent, const QmMathVector3f *position, const QmMathColour4f *colour, float radius, ApeLightType type, unsigned int flags )
{
	ApeLight *light = create_light( parent );

	ape_light_set_position( light, position );
	ape_light_set_colour( light, colour );

	light->type   = type;
	light->flags  = flags;
	light->radius = radius;
	light->angle  = 32.0f;

	return light;
}

static void destroy_light( void *data, ApeWorldNode *parent )
{
	ApeLight *self = data;
	if ( self == NULL )
	{
		return;
	}

	qm_os_memory_free( self );
}

static ApeWorldNode *clone_light( ApeWorldNode *src )
{
	ApeLight *srcLight = ( ApeLight * ) src;
	ApeLight *dstLight = ape_create_light( src->parent, &src->position, &srcLight->colour, srcLight->radius, srcLight->type, srcLight->flags );
	if ( dstLight == nullptr )
	{
		ape_console_warning_( "Failed to create light for duplication!\n" );
		return nullptr;
	}

	// sigh...
	APE_WORLD_NODE( dstLight )->angles = src->angles;

	dstLight->angle = srcLight->angle;
	dstLight->state = srcLight->state;

	return APE_WORLD_NODE( dstLight );
}

QmMathVector3f ape_light_get_direction( const ApeLight *self )
{
	QmMathVector3f angles = ape_light_get_angles( self );
	QmMathVector3f lightDirection;
	PlAnglesAxes( angles, nullptr, nullptr, &lightDirection );
	lightDirection = qm_math_vector3f_normalize( lightDirection );

	return lightDirection;
}

QmMathColour4f ape_light_get_colour( const ApeLight *self ) { return self->colour; }
void           ape_light_set_colour( ApeLight *light, const QmMathColour4f *colour ) { light->colour = *colour; }

QmMathVector3f ape_light_get_position( const ApeLight *self )
{
	return ape_world_node_get_position( APE_WORLD_NODE( self ) );
}

void ape_light_set_position( ApeLight *self, const QmMathVector3f *position )
{
	ape_world_node_set_position( ( ApeWorldNode * ) self, position );
}

QmMathVector3f ape_light_get_angles( const ApeLight *self )
{
	return ape_world_node_get_angles( ( ApeWorldNode * ) self );
}

void ape_light_set_angles( ApeLight *self, const QmMathVector3f *angles )
{
	ape_world_node_set_angles( ( ApeWorldNode * ) self, angles );
}

ApeLightType ape_light_get_type( const ApeLight *self )
{
	return self->type;
}

void ape_light_set_type( ApeLight *self, ApeLightType type )
{
	self->type = type;
}

void ape_light_set_radius( ApeLight *self, float radius )
{
	self->radius = radius;
}

ApeLightShadowType ape_light_get_shadow_type( const ApeLight *light )
{
	if ( ape_config_.renderer.forceShadows || light->flags & APE_LIGHT_FLAG_RUNTIME_SHADOWS || ( light->flags & APE_LIGHT_FLAG_DYNAMIC && light->flags & APE_LIGHT_FLAG_SHADOWS ) )
	{
		return APE_LIGHT_SHADOW_TYPE_DYNAMIC;
	}
	if ( light->flags & APE_LIGHT_FLAG_SHADOWS )
	{
		return APE_LIGHT_SHADOW_TYPE_STATIC;
	}

	return APE_LIGHT_SHADOW_TYPE_NONE;
}

bool ape_light_is_active( const ApeLight *light )
{
	if ( !( light->flags & APE_LIGHT_FLAG_ENABLED ) || light->colour.a <= 0.0f )
	{
		return false;
	}

	if ( light->type != APE_LIGHT_TYPE_SUN && light->radius <= 0.0f )
	{
		return false;
	}

	return true;
}

/**
 * Test if the plane will be hit by the light.
 */
bool ape_light_test_plane( const ApeLight *self, const PLCollisionPlane *plane )
{
	if ( self->type == APE_LIGHT_TYPE_SUN )
	{
		float dot = qm_math_vector3f_dot_product( plane->normal, ape_light_get_position( self ) );
		return dot <= 0;
	}

	QmMathVector3f dir = qm_math_vector3f_sub( plane->origin, ape_light_get_position( self ) );
	float          dot = qm_math_vector3f_dot_product( plane->normal, dir );
	return dot < 0;
}

bool ape_light_test_face( const ApeLight *self, const ApeBrushFace *face )
{
	// we're only doing this check for anything other than the sun, because
	// at the moment we don't really give a crap about the radius for sun sources
	// (and also because some of our rooms have it set to 0 and some don't, wheee...)
	if ( self->type != APE_LIGHT_TYPE_SUN && self->radius <= 0.0f )
	{
		return false;
	}

	if ( self->type != APE_LIGHT_TYPE_SUN && !com_collision_sphere_intersect_aabb( &( PLCollisionSphere ) { .origin = ape_light_get_position( self ), .radius = self->radius }, &face->bounds, nullptr ) )
	{
		return false;
	}

	QmMathVector3f dir;
	if ( self->type == APE_LIGHT_TYPE_SUN || self->type == APE_LIGHT_TYPE_SPOT )
	{
		dir = ape_light_get_direction( self );
	}
	else
	{
		dir = qm_math_vector3f_sub( face->bounds.absOrigin, ape_light_get_position( self ) );
	}

	float dot = qm_math_vector3f_dot_product( face->normal, dir );
	return dot < 0;
}

/**
 * Test if the plane will be shadowed by the light.
 */
bool ape_light_test_plane_shadow( const ApeLight *self, const ApeMaterial *material, const PLCollisionPlane *plane )
{
	if ( ape_light_get_shadow_type( self ) != APE_LIGHT_SHADOW_TYPE_DYNAMIC )
	{
		return false;
	}

	unsigned int flags = ape_material_get_flags_( material );

	return flags & APE_MATERIAL_FLAG_CAST_SHADOWS && !ape_light_test_plane( self, plane );
}

static AcmBranch *serialize_light( void *self, AcmBranch *root )
{
	ApeLight *light = self;
	acm_push_ui32( root, "type", light->type );
	com_acm_push_colour4f( root, "colour", &light->colour, true );
	acm_push_f32( root, "radius", light->radius );
	acm_push_f32( root, "angle", light->angle );
	acm_push_ui32( root, "flags", light->flags );
	acm_push_i32( root, "state", light->state );

	return root;
}

static ApeWorldNode *deserialize_light( ApeWorldNode *self, ApeWorldNode *parent, AcmBranch *root )
{
	ApeLight *light = ( ApeLight * ) self;
	light->type     = acm_get_uint( root, "type", light->type );
	light->colour   = com_acm_get_colour_f32( root, "colour", &light->colour );
	light->radius   = acm_get_f32( root, "radius", light->radius );
	light->angle    = acm_get_f32( root, "angle", light->angle );
	light->flags    = acm_get_uint( root, "flags", light->flags );
	light->state    = acm_get_int( root, "state", light->state );

	return self;
}

static void light_on_draw_editor( void *self, const bool isSelected )
{
	ApeLight *light = self;

	// this sucks, need to convert the colour due to inconsistency
	QmMathColour4ub colour = QM_MATH_COLOUR4F_TO_4UB( light->colour );

	QmMathVector3f position = ape_light_get_position( light );
	QmMathVector3f angles   = ape_light_get_angles( light );

	ApeLightType type = ape_light_get_type( light );

	if ( type == APE_LIGHT_TYPE_SUN || type == APE_LIGHT_TYPE_SPOT )
	{
		QmMathVector3f forward = ape_light_get_direction( light );
		QmMathVector3f end     = qm_math_vector3f_add( position, qm_math_vector3f_scale_float( forward, 16.0f + light->colour.a ) );
		ape_draw_debug_arrow( position, end, QM_MATH_COLOUR4F_TO_4UB( light->colour ), 1.0f );
	}

	if ( type == APE_LIGHT_TYPE_OMNI && isSelected )
	{
		ape_draw_debug_sphere( position, colour, light->radius );
	}
	else if ( type == APE_LIGHT_TYPE_SPOT && isSelected )
	{
		ape_draw_debug_cone(
		        position,
		        angles,
		        &colour,
		        light->radius,
		        light->angle,
		        16 );
	}
}

static ApePropertyEnum lightTypesEnum[] = {
        {"Omni", 0},
        {"Spot", 1},
        {"Sun",  2},
};

static ApePropertyBitFlag lightPropertyFlags[] = {
        {"Dynamic",         "Toggles dynamic state, otherwise uses baked lighting.", APE_LIGHT_FLAG_DYNAMIC        },
        {"Shadows",         "Toggles shadows.",                                      APE_LIGHT_FLAG_SHADOWS        },
        {"Runtime Shadows", "Toggles dynamic shadows.",                              APE_LIGHT_FLAG_RUNTIME_SHADOWS},
        {"Enabled",         "Toggles the light on or off.",                          APE_LIGHT_FLAG_ENABLED        },
        {"Flare",           "Toggles flare effect for light.",                       APE_LIGHT_FLAG_FLARE          },
};

static ApeProperty properties[] = {
        APE_PROPERTY_ENUM( "Type", "The type of light.", ApeLight, type, lightTypesEnum ),
        APE_PROPERTY_BASIC( "Radius", "Radius of the light.", ApeLight, radius, FLOAT ),
        APE_PROPERTY_BASIC( "Angle", "Angle of the light (spotlight only).", ApeLight, angle, FLOAT ),
        APE_PROPERTY_BASIC( "Colour", "Colour of the light.", ApeLight, colour, COLOUR ),
        APE_PROPERTY_BITFLAG( "Flags", "Variables flags for the light.", ApeLight, flags, lightPropertyFlags ),
};

const ApeWorldNodeClass ape_lightClass = {
        .identifier = "light",
        .magic      = QM_OS_MAGIC_TO_NUM( 'L', 'I', 'T', ' ' ),

        .create      = create_light,
        .destroy     = destroy_light,
        .serialize   = serialize_light,
        .deserialize = deserialize_light,

        .clone = clone_light,

#if !defined( APE_NO_EDITOR )

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),

        .editorIcon = "resources/new_light.gif",

        .onDrawEditor = light_on_draw_editor,

#endif
};
