// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

/////////////////////////////////////////////////////////////////
// World Environment State
/////////////////////////////////////////////////////////////////

#define TOX_WORLD_SECONDS_TO_MINUTE 60
#define TOX_WORLD_MINUTES_TO_HOUR   60
#define TOX_WORLD_HOURS_TO_DAY      24

typedef struct ToxWorldState {
	float windPower;
	PLVector3 windDirection;

	float waterHeight;

	unsigned int seconds;// not *real* seconds!
} ToxWorldState;

typedef enum ToxTimeOfDay {
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

static inline unsigned int toxGetTotalWorldSeconds( const ToxWorldState *simState ) {
	return simState->seconds;
}
static inline unsigned int toxGetTotalWorldMinutes( const ToxWorldState *simState ) { return simState->seconds / TOX_WORLD_SECONDS_TO_MINUTE; }
static inline unsigned int toxGetTotalWorldHours( const ToxWorldState *simState ) { return toxGetTotalWorldMinutes( simState ) / TOX_WORLD_MINUTES_TO_HOUR; }
static inline unsigned int toxGetTotalWorldDays( const ToxWorldState *simState ) { return toxGetTotalWorldHours( simState ) / TOX_WORLD_HOURS_TO_DAY; }

static inline unsigned int toxGetCurrentWorldSecond( const ToxWorldState *simState ) {
	return ( toxGetTotalWorldSeconds( simState ) - ( toxGetTotalWorldMinutes( simState ) / TOX_WORLD_SECONDS_TO_MINUTE ) ) % TOX_WORLD_SECONDS_TO_MINUTE;
}

static inline unsigned int toxGetCurrentWorldMinute( const ToxWorldState *simState ) {
	return ( toxGetTotalWorldMinutes( simState ) - ( toxGetTotalWorldHours( simState ) / TOX_WORLD_MINUTES_TO_HOUR ) ) % TOX_WORLD_MINUTES_TO_HOUR;
}

static inline unsigned int toxGetCurrentWorldHour( const ToxWorldState *simState ) {
	return ( toxGetTotalWorldHours( simState ) - ( toxGetTotalWorldDays( simState ) / TOX_WORLD_HOURS_TO_DAY ) ) % TOX_WORLD_HOURS_TO_DAY;
}

static inline ToxTimeOfDay toxGetWorldTimeOfDay( const ToxWorldState *simState ) {
	unsigned int hour = toxGetCurrentWorldHour( simState );
	if ( hour > TOX_WORLD_NIGHT_HOUR ) return TOX_ENV_TIMEOFDAY_NIGHT;
	if ( hour > TOX_WORLD_EVENING_HOUR ) return TOX_ENV_TIMEOFDAY_EVENING;
	if ( hour > TOX_WORLD_AFTERNOON_HOUR ) return TOX_ENV_TIMEOFDAY_AFTERNOON;
	if ( hour > TOX_WORLD_MORNING_HOUR ) return TOX_ENV_TIMEOFDAY_MORNING;
	if ( hour > TOX_WORLD_DAWN_HOUR ) return TOX_ENV_TIMEOFDAY_DAWN;
	return TOX_ENV_TIMEOFDAY_NIGHT;
}

void toxWorld_Spawn( void );
void toxWorld_Tick( void );
