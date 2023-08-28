// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

/////////////////////////////////////////////////////////////////
// World Environment State
/////////////////////////////////////////////////////////////////

#define MAG_SECONDS_TO_MINUTE 60
#define MAG_MINUTES_TO_HOUR   60
#define MAG_HOURS_TO_DAY      24

typedef struct MagWorldState {
	float windPower;
	PLVector3 windDirection;

	float waterHeight;

	unsigned int seconds;// not *real* seconds!
} MagWorldState;

typedef enum MagTimeOfDay {
	MAG_ENV_TIMEOFDAY_DAWN,
	MAG_ENV_TIMEOFDAY_MORNING,
	MAG_ENV_TIMEOFDAY_AFTERNOON,
	MAG_ENV_TIMEOFDAY_EVENING,
	MAG_ENV_TIMEOFDAY_NIGHT,

	MAG_ENV_MAX_TIMEOFDAY
} MagTimeOfDay;

#define MAG_NIGHT_HOUR     17
#define MAG_EVENING_HOUR   15
#define MAG_AFTERNOON_HOUR 12
#define MAG_MORNING_HOUR   9
#define MAG_DAWN_HOUR      5

static inline unsigned int magGetTotalWorldSeconds( const MagWorldState *simState ) {
	return simState->seconds;
}
static inline unsigned int magGetTotalWorldMinutes( const MagWorldState *simState ) { return simState->seconds / MAG_SECONDS_TO_MINUTE; }
static inline unsigned int magGetTotalWorldHours( const MagWorldState *simState ) { return magGetTotalWorldMinutes( simState ) / MAG_MINUTES_TO_HOUR; }
static inline unsigned int magGetTotalWorldDays( const MagWorldState *simState ) { return magGetTotalWorldHours( simState ) / MAG_HOURS_TO_DAY; }

static inline unsigned int magGetCurrentWorldSecond( const MagWorldState *simState ) {
	return ( magGetTotalWorldSeconds( simState ) - ( magGetTotalWorldMinutes( simState ) / MAG_SECONDS_TO_MINUTE ) ) % MAG_SECONDS_TO_MINUTE;
}

static inline unsigned int magGetCurrentWorldMinute( const MagWorldState *simState ) {
	return ( magGetTotalWorldMinutes( simState ) - ( magGetTotalWorldHours( simState ) / MAG_MINUTES_TO_HOUR ) ) % MAG_MINUTES_TO_HOUR;
}

static inline unsigned int magGetCurrentWorldHour( const MagWorldState *simState ) {
	return ( magGetTotalWorldHours( simState ) - ( magGetTotalWorldDays( simState ) / MAG_HOURS_TO_DAY ) ) % MAG_HOURS_TO_DAY;
}

static inline MagTimeOfDay magGetWorldTimeOfDay( const MagWorldState *simState ) {
	unsigned int hour = magGetCurrentWorldHour( simState );
	if ( hour > MAG_NIGHT_HOUR ) return MAG_ENV_TIMEOFDAY_NIGHT;
	if ( hour > MAG_EVENING_HOUR ) return MAG_ENV_TIMEOFDAY_EVENING;
	if ( hour > MAG_AFTERNOON_HOUR ) return MAG_ENV_TIMEOFDAY_AFTERNOON;
	if ( hour > MAG_MORNING_HOUR ) return MAG_ENV_TIMEOFDAY_MORNING;
	if ( hour > MAG_DAWN_HOUR ) return MAG_ENV_TIMEOFDAY_DAWN;
	return MAG_ENV_TIMEOFDAY_NIGHT;
}
