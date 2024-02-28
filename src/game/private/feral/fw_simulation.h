// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/////////////////////////////////////////////////////////////////
// World Simulation
/////////////////////////////////////////////////////////////////

#define FW_SIM_SECONDS_TO_MINUTE 60
#define FW_SIM_MINUTES_TO_HOUR   60
#define FW_SIM_HOURS_TO_DAY      24

typedef struct FWSimState
{
	float windPower;
	PLVector3 windDirection;

	float waterHeight;

	unsigned int seconds;// not *real* seconds!
} FWSimState;

typedef enum FWSimTimeOfDay
{
	FW_TIMEOFDAY_DAWN,
	FW_TIMEOFDAY_MORNING,
	FW_TIMEOFDAY_AFTERNOON,
	FW_TIMEOFDAY_EVENING,
	FW_TIMEOFDAY_NIGHT,

	FW_MAX_TIMEOFDAY
} FWSimTimeOfDay;

static inline unsigned int FW_Sim_GetTotalSeconds( const FWSimState *simState ) { return simState->seconds; }
static inline unsigned int FW_Sim_GetTotalMinutes( const FWSimState *simState ) { return simState->seconds / FW_SIM_SECONDS_TO_MINUTE; }
static inline unsigned int FW_Sim_GetTotalHours( const FWSimState *simState ) { return FW_Sim_GetTotalMinutes( simState ) / FW_SIM_MINUTES_TO_HOUR; }
static inline unsigned int FW_Sim_GetTotalDays( const FWSimState *simState ) { return FW_Sim_GetTotalHours( simState ) / FW_SIM_HOURS_TO_DAY; }

static inline unsigned int FW_Sim_GetCurrentSecond( const FWSimState *simState )
{
	return ( FW_Sim_GetTotalSeconds( simState ) - ( FW_Sim_GetTotalMinutes( simState ) / FW_SIM_SECONDS_TO_MINUTE ) ) % FW_SIM_SECONDS_TO_MINUTE;
}

static inline unsigned int FW_Sim_GetCurrentMinute( const FWSimState *simState )
{
	return ( FW_Sim_GetTotalMinutes( simState ) - ( FW_Sim_GetTotalHours( simState ) / FW_SIM_MINUTES_TO_HOUR ) ) % FW_SIM_MINUTES_TO_HOUR;
}

static inline unsigned int FW_Sim_GetCurrentHour( const FWSimState *simState )
{
	return ( FW_Sim_GetTotalHours( simState ) - ( FW_Sim_GetTotalDays( simState ) / FW_SIM_HOURS_TO_DAY ) ) % FW_SIM_HOURS_TO_DAY;
}

static inline FWSimTimeOfDay FW_Sim_GetTimeOfDay( const FWSimState *simState )
{
	unsigned int hour = FW_Sim_GetCurrentHour( simState );
	if ( hour > 17 ) return FW_TIMEOFDAY_NIGHT;
	if ( hour > 15 ) return FW_TIMEOFDAY_EVENING;
	if ( hour > 12 ) return FW_TIMEOFDAY_AFTERNOON;
	if ( hour > 9 ) return FW_TIMEOFDAY_MORNING;
	if ( hour > 5 ) return FW_TIMEOFDAY_DAWN;
	return FW_TIMEOFDAY_NIGHT;
}
