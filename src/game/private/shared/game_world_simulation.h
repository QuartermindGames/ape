// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/////////////////////////////////////////////////////////////////
// World Environment State
/////////////////////////////////////////////////////////////////

typedef struct WorldSimulationConfig
{
	unsigned int secondsToMinute;
	unsigned int minutesToHour;
	unsigned int hoursToDay;

	// different hours for different times of day
	unsigned int nightHour;
	unsigned int eveningHour;
	unsigned int afternoonHour;
	unsigned int morningHour;
	unsigned int dawnHour;
} WorldSimulationConfig;

typedef enum WorldSimulationTime
{
	WORLD_SIMULATION_TIME_DAWN,
	WORLD_SIMULATION_TIME_MORNING,
	WORLD_SIMULATION_TIME_AFTERNOON,
	WORLD_SIMULATION_TIME_EVENING,
	WORLD_SIMULATION_TIME_NIGHT,
} WorldSimulationTime;

typedef struct WorldSimulation
{
	WorldSimulationConfig config;

	// these will just be computed based on config
	unsigned int secondsToHour;
	unsigned int secondsToDay;

	unsigned int seconds;// not *real* seconds!

	float speed;
} WorldSimulation;

static void world_simulation_initialize( WorldSimulation *simulation )
{
	// setup some defaults...
	simulation->config.nightHour = 17;
}

static inline unsigned int world_simulation_get_total_seconds( const WorldSimulation *simulation ) { return simulation->seconds; }
static inline unsigned int world_simulation_get_total_minutes( const WorldSimulation *simulation ) { return simulation->seconds / simulation->config.secondsToMinute; }
static inline unsigned int world_simulation_get_total_hours( const WorldSimulation *simulation ) { return world_simulation_get_total_minutes( simulation ) / simulation->config.minutesToHour; }
static inline unsigned int world_simulation_get_total_days( const WorldSimulation *simulation ) { return world_simulation_get_total_hours( simulation ) / simulation->config.hoursToDay; }

static inline unsigned int world_simulation_get_current_second( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_seconds( simulation ) - ( world_simulation_get_total_minutes( simulation ) / simulation->config.secondsToMinute ) ) % simulation->config.secondsToMinute;
}

static inline unsigned int world_simulation_get_current_minute( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_minutes( simulation ) - ( world_simulation_get_total_hours( simulation ) / simulation->config.minutesToHour ) ) % simulation->config.minutesToHour;
}

static inline unsigned int world_simulation_get_current_hour( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_hours( simulation ) - ( world_simulation_get_total_days( simulation ) / simulation->config.hoursToDay ) ) % simulation->config.hoursToDay;
}

/// This returns the total number of seconds for the current day.
static inline unsigned int world_simulation_get_seconds_in_day( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_seconds( simulation ) ) % simulation->secondsToDay;
}

static inline WorldSimulationTime world_simulation_get_time_of_day( const WorldSimulation *simulation )
{
	unsigned int hour = world_simulation_get_current_hour( simulation );
	if ( hour > simulation->config.nightHour ) return WORLD_SIMULATION_TIME_NIGHT;
	if ( hour > simulation->config.eveningHour ) return WORLD_SIMULATION_TIME_EVENING;
	if ( hour > simulation->config.afternoonHour ) return WORLD_SIMULATION_TIME_AFTERNOON;
	if ( hour > simulation->config.morningHour ) return WORLD_SIMULATION_TIME_MORNING;
	if ( hour > simulation->config.dawnHour ) return WORLD_SIMULATION_TIME_DAWN;
	return WORLD_SIMULATION_TIME_NIGHT;
}

static inline const char *world_get_time_of_day_descriptor( WorldSimulationTime time )
{
	switch ( time )
	{
		case WORLD_SIMULATION_TIME_DAWN: return "dawn";
		case WORLD_SIMULATION_TIME_MORNING: return "morning";
		case WORLD_SIMULATION_TIME_AFTERNOON: return "afternoon";
		case WORLD_SIMULATION_TIME_EVENING: return "evening";
		case WORLD_SIMULATION_TIME_NIGHT:
		default: return "night";
	}
}

static inline void world_simulation_tick( const WorldSimulation *simulation )
{
}
