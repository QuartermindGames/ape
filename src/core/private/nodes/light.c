// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Lights
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "../client/renderer/renderer.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeLight *ape_create_light( ApeWorldNode *parent, const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, unsigned int flags )
{
	ApeLight *light = PL_NEW( ApeLight );
	ape_world_node_create( parent, APE_WORLD_NODE_TYPE_LIGHT, position, &pl_vecOrigin3, light );

	light->colour = *colour;
	light->type = type;
	light->flags = flags;
	light->radius = radius;

	return light;
}

void ape_light_destroy_( void *data )
{
	ApeLight *self = ( ApeLight * ) data;
	if ( self == NULL )
	{
		return;
	}

	PL_DELETE( self );
}

PLColourF32 ape_light_get_colour( const ApeLight *light ) { return light->colour; }
void ape_light_set_colour( ApeLight *light, const PLColourF32 *colour ) { light->colour = *colour; }

PLVector3 ape_light_get_position( const ApeLight *self ) { return self->header.node->position; }
void ape_light_set_position( ApeLight *self, const PLVector3 *position ) { ape_world_node_set_position( self->header.node, position ); }

PLVector3 ape_light_get_angles( const ApeLight *self ) { return self->header.node->angles; }
void ape_light_set_angles( ApeLight *self, const PLVector3 *angles ) { ape_world_node_set_angles( self->header.node, angles ); }

ApeLightShadowType ape_light_get_shadow_type( const ApeLight *light )
{
	if ( ape_config_.renderer.forceShadows || ( light->flags & SS_ARL_LIGHT_FLAG_RUNTIME_SHADOWS || ( light->flags & SS_ARL_LIGHT_FLAG_DYNAMIC && light->flags & SS_ARL_LIGHT_FLAG_SHADOWS ) ) )
	{
		return SS_APE_LIGHT_SHADOW_TYPE_DYNAMIC;
	}
	else if ( light->flags & SS_ARL_LIGHT_FLAG_SHADOWS )
	{
		return SS_APE_LIGHT_SHADOW_TYPE_STATIC;
	}

	return SS_APE_LIGHT_SHADOW_TYPE_NONE;
}

bool ape_light_is_active( const ApeLight *light )
{
	if ( !( light->flags & SS_ARL_LIGHT_FLAG_ENABLED ) || light->colour.a <= 0.0f )
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
	PLVector3 pos = ape_light_get_position( self );
	PLVector3 dir = PlNormalizeVector3( PlSubtractVector3( plane->origin, pos ) );
	if ( PlVector3DotProduct( plane->normal, dir ) >= 0 )
	{
		return true;
	}

	return false;
}

/**
 * Test if the plane will be shadowed by the light.
 */
bool ape_light_test_plane_shadow( const ApeLight *self, const ApeMaterial *material, const PLCollisionPlane *plane )
{
	if ( ape_light_get_shadow_type( self ) != SS_APE_LIGHT_SHADOW_TYPE_DYNAMIC )
	{
		return false;
	}

	return ( ape_material_shadows_enabled( material ) && !ape_light_test_plane( self, plane ) );
}
