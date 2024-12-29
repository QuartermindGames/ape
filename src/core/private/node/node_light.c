// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Lights
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "renderer/renderer.h"

ApeLight *ape_create_light( ApeWorldNode *parent, const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, unsigned int flags )
{
	ApeLight *light = PL_NEW( ApeLight );
	ape_world_node_setup_( &light->base, parent, APE_WORLD_NODE_TYPE_LIGHT, nullptr, position, &pl_vecOrigin3 );

	light->colour = *colour;
	light->type   = type;
	light->flags  = flags;
	light->radius = radius;

	return light;
}

void ape_light_destroy_( void *data, ApeWorldNode *parent )
{
	ApeLight *self = ( ApeLight * ) data;
	if ( self == NULL )
	{
		return;
	}

	PL_DELETE( self );
}

PLColourF32 ape_light_get_colour( const ApeLight *light ) { return light->colour; }
void        ape_light_set_colour( ApeLight *light, const PLColourF32 *colour ) { light->colour = *colour; }

PLVector3 ape_light_get_position( const ApeLight *self )
{
	return ape_world_node_get_position( ( ApeWorldNode * ) self );
}

void ape_light_set_position( ApeLight *self, const PLVector3 *position )
{
	ape_world_node_set_position( ( ApeWorldNode * ) self, position );
}

PLVector3 ape_light_get_angles( const ApeLight *self )
{
	return ape_world_node_get_angles( ( ApeWorldNode * ) self );
}

void ape_light_set_angles( ApeLight *self, const PLVector3 *angles )
{
	ape_world_node_set_angles( ( ApeWorldNode * ) self, angles );
}

void ape_light_set_radius( ApeLight *self, float radius )
{
	self->radius = radius;
}

ApeLightShadowType ape_light_get_shadow_type( const ApeLight *light )
{
	if ( ape_config_.renderer.forceShadows || ( light->flags & APE_LIGHT_FLAG_RUNTIME_SHADOWS || ( light->flags & APE_LIGHT_FLAG_DYNAMIC && light->flags & APE_LIGHT_FLAG_SHADOWS ) ) )
	{
		return APE_LIGHT_SHADOW_TYPE_DYNAMIC;
	}
	else if ( light->flags & APE_LIGHT_FLAG_SHADOWS )
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
	//HACK: the sun is an awkward case...
	PLVector3 origin = ( self->type == APE_LIGHT_TYPE_SUN ) ? pl_vecOrigin3 : plane->origin;

	PLVector3 dir = PlNormalizeVector3( PlSubtractVector3( origin, ape_light_get_position( self ) ) );
	float     dot = PlVector3DotProduct( plane->normal, dir );

	if ( self->type == APE_LIGHT_TYPE_SUN )
	{
		return ( dot >= 0 );
	}

	return ( dot < 0 );
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

	uint flags = ape_material_get_flags( material );

	return ( ( flags & APE_MATERIAL_FLAG_CAST_SHADOWS ) && !ape_light_test_plane( self, plane ) );
}

bool ape_light_is_visible( const ApeLight *self, const ApeCamera *camera )
{
	if ( self->type == APE_LIGHT_TYPE_SUN )
	{
		return true;
	}

	PLVector3         lightPosition = ape_light_get_position( self );
	PLCollisionSphere sphere        = PlSetupCollisionSphere( lightPosition, self->radius );
	return PlgIsSphereInsideView( camera->internal, &sphere );
}

static AcmBranch *serialize_light( void *self, AcmBranch *root )
{
	ApeLight *light = self;
	acm_push_ui32( root, "type", light->type );
	acm_push_colour4f( root, "colour", &light->colour, true );
	acm_push_f32( root, "radius", light->radius );
	acm_push_bool( root, "isHidden", light->isHidden );
	acm_push_ui32( root, "flags", light->flags );
	acm_push_i32( root, "state", light->state );

	return root;
}

static ApeWorldNode *deserialize_light( ApeWorldNode *parent, AcmBranch *root )
{
	ApeLight *light = ape_create_light( parent, &pl_vecOrigin3, &PL_COLOURF32( 1.0f, 1.0f, 1.0f, 0.0f ), 0.0f, APE_LIGHT_TYPE_OMNI, 0 );
	light->type     = acm_get_uint( root, "type", light->type );
	light->colour   = acm_get_colour_f32( root, "colour", &light->colour );
	light->radius   = acm_get_f32( root, "radius", light->radius );
	light->isHidden = acm_get_bool( root, "isHidden", light->isHidden );
	light->flags    = acm_get_uint( root, "flags", light->flags );
	light->state    = acm_branch_get_child_int( root, "state", light->state );
	return APE_WORLD_NODE( light );
}

static ApeWorldNodePropertyEnum lightTypesEnum[] = {
        {"Omni", 0},
        {"Spot", 1},
        {"Sun",  2},
};

static ApeWorldNodeProperty properties[] = {
        APE_WORLD_NODE_PROPERTY_ENUM( "Type", "The type of light.", ApeLight, type, lightTypesEnum ),
        APE_WORLD_NODE_PROPERTY_BASIC( "Radius", "Radius of the light.", ApeLight, radius, FLOAT ),
        APE_WORLD_NODE_PROPERTY_BASIC( "Colour", "Colour of the light.", ApeLight, colour, COLOUR ),
};

const ApeWorldNodeClass ape_lightClass = {
        .identifier          = "light",
        .magic               = PL_MAGIC_TO_NUM( 'L', 'I', 'T', ' ' ),
        .destroyFunction     = ape_light_destroy_,
        .serializeFunction   = serialize_light,
        .deserializeFunction = deserialize_light,

        .properties    = properties,
        .numProperties = PL_ARRAY_ELEMENTS( properties ),
};
