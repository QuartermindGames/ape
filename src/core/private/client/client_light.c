// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Lights
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "renderer/renderer.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeLight *ape_light_create( const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, unsigned int flags )
{
	ApeLight *light = PL_NEW( ApeLight );

	ape_world_node_setup_header( &light->header, APE_WORLD_NODE_TYPE_LIGHT );

	light->position = *position;
	light->colour = *colour;
	light->type = type;
	light->flags = flags;
	light->radius = radius;

	return light;
}

void ape_light_destroy( ApeLight *light )
{
	if ( light == NULL )
	{
		return;
	}

	PL_DELETE( light );
}

PLColourF32 ss_arl_light_get_colour( const ApeLight *light ) { return light->colour; }
void ss_arl_light_set_colour( ApeLight *light, const PLColourF32 *colour ) { light->colour = *colour; }

PLVector3 ss_arl_light_get_position( const ApeLight *light ) { return light->position; }
void ss_arl_light_set_position( ApeLight *light, const PLVector3 *position ) { light->position = *position; }

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
