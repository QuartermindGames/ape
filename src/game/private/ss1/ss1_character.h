// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Pawns represent anything *living* in the world.

#pragma once

typedef enum PMPawnSex : uint8_t
{
	PM_PAWN_SEX_MALE,
	PM_PAWN_SEX_FEMALE,
} PMPawnSex;

typedef enum PMPawnPhase : uint8_t
{
	PM_PAWN_PHASE_BABY,
	PM_PAWN_PHASE_CHILD,
	PM_PAWN_PHASE_ADULT,
	PM_PAWN_PHASE_ELDERLY,
	PM_PAWN_PHASE_DEAD,

	PM_PAWN_MAX_PHASES
} PMPawnPhase;

typedef struct PMPawn
{
	PMPawnSex sex;
	int16_t stamina;

	uint8_t age, maxAge;

	uint32_t experience;
	uint8_t level;
} PMPawn;

static inline PMPawnPhase pm_pawn_determine_phase( const PMPawn *self )
{
	//todo: check health, if below or equal to 0, they're dead!
	if ( self->age >= self->maxAge )
	{
		return PM_PAWN_PHASE_DEAD;
	}

	uint8_t m = self->maxAge / PM_PAWN_MAX_PHASES;
	if ( self->age >= ( 60 - m ) )
	{
		return PM_PAWN_PHASE_ELDERLY;
	}
	else if ( self->age >= ( 16 - m ) )
	{
		return PM_PAWN_PHASE_ADULT;
	}
	else if ( self->age >= ( 5 - m ) )
	{
		return PM_PAWN_PHASE_CHILD;
	}

	return PM_PAWN_PHASE_CHILD;
}

/**
 * Returns the amount of XP required to make
 * it to the next level.
 */
static inline uint32_t pm_pawn_next_level( const PMPawn *self )
{
	return ( uint32_t ) round( 100 * ( self->level ^ 3 ) );
}

/**
 * Determines whether or not two pawns can procreate.
 */
static inline bool pm_pawn_can_breed( const PMPawn *self, const PMPawn *other )
{
	if ( self->sex == other->sex )
	{
		return false;
	}

	return true;
}
