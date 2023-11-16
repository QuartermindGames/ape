// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Lights
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "renderer/renderer.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

SS_Arl_Light *ape_light_create( const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, unsigned int flags )
{
	SS_Arl_Light *light = PL_NEW( SS_Arl_Light );
	light->position = *position;
	light->colour = *colour;
	light->type = type;
	light->flags = flags;
	light->radius = radius;

	return light;
}

void ape_light_destroy( SS_Arl_Light *light )
{
}

PLColourF32 ape_light_get_colour( const SS_Arl_Light *light ) { return light->colour; }
void ape_light_set_colour( SS_Arl_Light *light, const PLColourF32 *colour ) { light->colour = *colour; }

PLVector3 ape_light_get_position( const SS_Arl_Light *light ) { return light->position; }
void ape_light_set_position( SS_Arl_Light *light, const PLVector3 *position ) { light->position = *position; }
