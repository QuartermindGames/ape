// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: A light manager that makes the light turn on at night.
// Author:  Mark E. Sowden

#include "../tox_game.h"
#include "../tox_world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct ToxLightNight
{
	ApeLight *light;
} ToxLightNight;

/////////////////////////////////////////////////////////////////////////////////////
// Public

const AclEntityClassDefinition *tox_light_night_get_class_table( void )
{
	static AclEntityClassDefinition table;
	PL_ZERO_( table );

	table.name = "tox_light_night";

	return &table;
}
