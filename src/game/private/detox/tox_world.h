// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/////////////////////////////////////////////////////////////////
// World Environment State
/////////////////////////////////////////////////////////////////

#define TOX_WORLD_SECONDS_TO_MINUTE 60
#define TOX_WORLD_MINUTES_TO_HOUR   60
#define TOX_WORLD_HOURS_TO_DAY      24

#define TOX_WORLD_SECONDS_TO_HOUR ( TOX_WORLD_SECONDS_TO_MINUTE * TOX_WORLD_MINUTES_TO_HOUR )
#define TOX_WORLD_SECONDS_TO_DAY  ( TOX_WORLD_SECONDS_TO_HOUR * TOX_WORLD_HOURS_TO_DAY )

typedef struct ToxWorldState
{
	float windPower;
	PLVector3 windDirection;

	unsigned int seconds;// not *real* seconds!
} ToxWorldState;

typedef enum ToxTimeOfDay
{
	TOX_ENV_TIMEOFDAY_DAWN,
	TOX_ENV_TIMEOFDAY_MORNING,
	TOX_ENV_TIMEOFDAY_AFTERNOON,
	TOX_ENV_TIMEOFDAY_EVENING,
	TOX_ENV_TIMEOFDAY_NIGHT,

	TOX_ENV_MAX_TIMEOFDAY
} ToxTimeOfDay;

#define TOX_WORLD_NIGHT_HOUR     17
#define TOX_WORLD_EVENING_HOUR   15
#define TOX_WORLD_AFTERNOON_HOUR 12
#define TOX_WORLD_MORNING_HOUR   9
#define TOX_WORLD_DAWN_HOUR      5

static inline unsigned int tox_world_get_total_seconds( const ToxWorldState *worldState ) { return worldState->seconds; }
static inline unsigned int tox_world_get_total_minutes( const ToxWorldState *worldState ) { return worldState->seconds / TOX_WORLD_SECONDS_TO_MINUTE; }
static inline unsigned int tox_world_get_total_hours( const ToxWorldState *worldState ) { return tox_world_get_total_minutes( worldState ) / TOX_WORLD_MINUTES_TO_HOUR; }
static inline unsigned int tox_world_get_total_days( const ToxWorldState *worldState ) { return tox_world_get_total_hours( worldState ) / TOX_WORLD_HOURS_TO_DAY; }

static inline unsigned int tox_world_get_current_second( const ToxWorldState *worldState )
{
	return ( tox_world_get_total_seconds( worldState ) - ( tox_world_get_total_minutes( worldState ) / TOX_WORLD_SECONDS_TO_MINUTE ) ) % TOX_WORLD_SECONDS_TO_MINUTE;
}

static inline unsigned int tox_world_get_current_minute( const ToxWorldState *worldState )
{
	return ( tox_world_get_total_minutes( worldState ) - ( tox_world_get_total_hours( worldState ) / TOX_WORLD_MINUTES_TO_HOUR ) ) % TOX_WORLD_MINUTES_TO_HOUR;
}

static inline unsigned int tox_world_get_current_hour( const ToxWorldState *worldState )
{
	return ( tox_world_get_total_hours( worldState ) - ( tox_world_get_total_days( worldState ) / TOX_WORLD_HOURS_TO_DAY ) ) % TOX_WORLD_HOURS_TO_DAY;
}

/// This returns the total number of seconds for the current day.
static inline unsigned int tox_world_get_seconds_in_day( const ToxWorldState *worldState )
{
	return ( tox_world_get_total_seconds( worldState ) ) % TOX_WORLD_SECONDS_TO_DAY;
}

static inline ToxTimeOfDay tox_world_get_time_of_day( const ToxWorldState *worldState )
{
	unsigned int hour = tox_world_get_current_hour( worldState );
	if ( hour > TOX_WORLD_NIGHT_HOUR ) return TOX_ENV_TIMEOFDAY_NIGHT;
	if ( hour > TOX_WORLD_EVENING_HOUR ) return TOX_ENV_TIMEOFDAY_EVENING;
	if ( hour > TOX_WORLD_AFTERNOON_HOUR ) return TOX_ENV_TIMEOFDAY_AFTERNOON;
	if ( hour > TOX_WORLD_MORNING_HOUR ) return TOX_ENV_TIMEOFDAY_MORNING;
	if ( hour > TOX_WORLD_DAWN_HOUR ) return TOX_ENV_TIMEOFDAY_DAWN;
	return TOX_ENV_TIMEOFDAY_NIGHT;
}

static inline const char *tox_world_get_time_of_day_descriptor( ToxTimeOfDay timeOfDay )
{
	switch ( timeOfDay )
	{
		case TOX_ENV_TIMEOFDAY_DAWN: return "dawn";
		case TOX_ENV_TIMEOFDAY_MORNING: return "morning";
		case TOX_ENV_TIMEOFDAY_AFTERNOON: return "afternoon";
		case TOX_ENV_TIMEOFDAY_EVENING: return "evening";
		case TOX_ENV_TIMEOFDAY_NIGHT:
		default: return "night";
	}
}

ToxWorldState *tox_world_get_state( void );

void tox_world_spawn( ApeWorld *world );
void tox_world_tick( ApeWorld *world );

float tox_world_get_sun_brightness( void );
float tox_world_get_moon_brightness( void );
