// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Lights
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "renderer/renderer.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

SSArlLight *ss_arl_light_create( const PLVector3 *position, const PLColourF32 *colour, float radius, SSArlLightType type, unsigned int flags )
{
	SSArlLight *light = PL_NEW( SSArlLight );
	light->position = *position;
	light->colour = *colour;
	light->type = type;
	light->flags = flags;
	light->radius = radius;

	return light;
}

void ss_arl_light_destroy( SSArlLight *light )
{
}

PLColourF32 ss_arl_light_get_colour( const SSArlLight *light ) { return light->colour; }
void ss_arl_light_set_colour( SSArlLight *light, const PLColourF32 *colour ) { light->colour = *colour; }

PLVector3 ss_arl_light_get_position( const SSArlLight *light ) { return light->position; }
void ss_arl_light_set_position( SSArlLight *light, const PLVector3 *position ) { light->position = *position; }

SSApeLightShadowType ss_ape_light_get_shadow_type( const SSArlLight *light )
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
