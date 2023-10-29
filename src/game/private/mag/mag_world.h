// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

/////////////////////////////////////////////////////////////////
// World Environment State
/////////////////////////////////////////////////////////////////

#define MAG_WORLD_SECONDS_TO_MINUTE 60
#define MAG_WORLD_MINUTES_TO_HOUR   60
#define MAG_WORLD_HOURS_TO_DAY      24

#define MAG_WORLD_SECONDS_TO_HOUR ( MAG_WORLD_SECONDS_TO_MINUTE * MAG_WORLD_MINUTES_TO_HOUR )
#define MAG_WORLD_SECONDS_TO_DAY  ( MAG_WORLD_SECONDS_TO_HOUR * MAG_WORLD_HOURS_TO_DAY )

typedef struct MagWorldState
{
	float windPower;
	PLVector3 windDirection;

	unsigned int seconds;// not *real* seconds!
} MagWorldState;

typedef enum ToxTimeOfDay
{
	TOX_ENV_TIMEOFDAY_DAWN,
	TOX_ENV_TIMEOFDAY_MORNING,
	TOX_ENV_TIMEOFDAY_AFTERNOON,
	TOX_ENV_TIMEOFDAY_EVENING,
	TOX_ENV_TIMEOFDAY_NIGHT,

	TOX_ENV_MAX_TIMEOFDAY
} ToxTimeOfDay;

#define MAG_WORLD_NIGHT_HOUR     17
#define MAG_WORLD_EVENING_HOUR   15
#define MAG_WORLD_AFTERNOON_HOUR 12
#define MAG_WORLD_MORNING_HOUR   9
#define MAG_WORLD_DAWN_HOUR      5

static inline unsigned int mag_world_get_total_seconds( const MagWorldState *worldState ) { return worldState->seconds; }
static inline unsigned int mag_world_get_total_minutes( const MagWorldState *worldState ) { return worldState->seconds / MAG_WORLD_SECONDS_TO_MINUTE; }
static inline unsigned int mag_world_get_total_hours( const MagWorldState *worldState ) { return mag_world_get_total_minutes( worldState ) / MAG_WORLD_MINUTES_TO_HOUR; }
static inline unsigned int mag_world_get_total_days( const MagWorldState *worldState ) { return mag_world_get_total_hours( worldState ) / MAG_WORLD_HOURS_TO_DAY; }

static inline unsigned int mag_world_get_current_second( const MagWorldState *worldState )
{
	return ( mag_world_get_total_seconds( worldState ) - ( mag_world_get_total_minutes( worldState ) / MAG_WORLD_SECONDS_TO_MINUTE ) ) % MAG_WORLD_SECONDS_TO_MINUTE;
}

static inline unsigned int mag_world_get_current_minute( const MagWorldState *worldState )
{
	return ( mag_world_get_total_minutes( worldState ) - ( mag_world_get_total_hours( worldState ) / MAG_WORLD_MINUTES_TO_HOUR ) ) % MAG_WORLD_MINUTES_TO_HOUR;
}

static inline unsigned int mag_world_get_current_hour( const MagWorldState *worldState )
{
	return ( mag_world_get_total_hours( worldState ) - ( mag_world_get_total_days( worldState ) / MAG_WORLD_HOURS_TO_DAY ) ) % MAG_WORLD_HOURS_TO_DAY;
}

/// This returns the total number of seconds for the current day.
static inline unsigned int mag_world_get_seconds_in_day( const MagWorldState *worldState )
{
	return ( mag_world_get_total_seconds( worldState ) ) % MAG_WORLD_SECONDS_TO_DAY;
}

static inline ToxTimeOfDay mag_world_get_time_of_day( const MagWorldState *worldState )
{
	unsigned int hour = mag_world_get_current_hour( worldState );
	if ( hour > MAG_WORLD_NIGHT_HOUR ) return TOX_ENV_TIMEOFDAY_NIGHT;
	if ( hour > MAG_WORLD_EVENING_HOUR ) return TOX_ENV_TIMEOFDAY_EVENING;
	if ( hour > MAG_WORLD_AFTERNOON_HOUR ) return TOX_ENV_TIMEOFDAY_AFTERNOON;
	if ( hour > MAG_WORLD_MORNING_HOUR ) return TOX_ENV_TIMEOFDAY_MORNING;
	if ( hour > MAG_WORLD_DAWN_HOUR ) return TOX_ENV_TIMEOFDAY_DAWN;
	return TOX_ENV_TIMEOFDAY_NIGHT;
}

static inline const char *mag_world_get_time_of_day_descriptor( ToxTimeOfDay timeOfDay )
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

const MagWorldState *mag_world_get_state( void );

void mag_world_spawn( ApeWorld *world );
void mag_world_tick( void );

float mag_world_get_sun_brightness( void );
float mag_world_get_moon_brightness( void );
