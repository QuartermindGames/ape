// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Creature implementation.
// Author:  Mark E. Sowden

#include "mag_game.h"
#include "mag_creature.h"

/////////////////////////////////////////////////////////////////////////////////////
// Public

bool mag_creature_can_become_pregnant( const MagCreature *creature )
{
	if ( creature->sex == MAG_CREATURE_SEX_MALE || creature->isPregnant )
		return false;

	if ( creature->phase != MAG_CREATURE_LIFE_PHASE_ADULTHOOD )
		return false;

	return false;//todo
}

bool mag_creature_can_breed( const MagCreature *creature, const MagCreature *otherCreature )
{
	// Can't get pregnant if we're already pregnant!
	if ( creature->isPregnant || otherCreature->isPregnant )
		return false;

	// Only adults can breed
	if ( creature->phase < MAG_CREATURE_LIFE_PHASE_ADULTHOOD || otherCreature->phase < MAG_CREATURE_LIFE_PHASE_ADULTHOOD )
		return false;

	switch ( creature->sex )
	{
		default:
			return false;
		case MAG_CREATURE_SEX_INTERSEX:
			return true;
		case MAG_CREATURE_SEX_MALE:
			return ( otherCreature->sex == MAG_CREATURE_SEX_FEMALE );
		case MAG_CREATURE_SEX_FEMALE:
			return ( otherCreature->sex == MAG_CREATURE_SEX_MALE );
	}
}
