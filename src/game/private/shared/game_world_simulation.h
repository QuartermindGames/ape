// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/////////////////////////////////////////////////////////////////////////////////////
// World Environment State
/////////////////////////////////////////////////////////////////////////////////////

typedef struct WorldSimulationConfig
{
	uint secondsToMinute;
	uint minutesToHour;
	uint hoursToDay;

	// different hours for different times of day
	uint nightHour;
	uint eveningHour;
	uint afternoonHour;
	uint morningHour;
	uint dawnHour;
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

	uint seconds;// not *real* seconds!
	uint speedMultiplier;
} WorldSimulation;

static void world_simulation_initialize( WorldSimulation *simulation )
{
	// setup some defaults...
	simulation->config.nightHour = 17;

	simulation->config.secondsToMinute = 60;
	simulation->config.minutesToHour   = 60;
	simulation->config.hoursToDay      = 24;

	simulation->speedMultiplier = 128.0f;
}

static inline uint world_simulation_get_seconds_to_hour( const WorldSimulation *simulation ) { return simulation->config.secondsToMinute * simulation->config.minutesToHour; }
static inline uint world_simulation_get_seconds_to_day( const WorldSimulation *simulation ) { return world_simulation_get_seconds_to_hour( simulation ) * simulation->config.hoursToDay; }

static inline uint world_simulation_get_total_seconds( const WorldSimulation *simulation ) { return simulation->seconds; }
static inline uint world_simulation_get_total_minutes( const WorldSimulation *simulation ) { return simulation->seconds / simulation->config.secondsToMinute; }
static inline uint world_simulation_get_total_hours( const WorldSimulation *simulation ) { return world_simulation_get_total_minutes( simulation ) / simulation->config.minutesToHour; }
static inline uint world_simulation_get_total_days( const WorldSimulation *simulation ) { return world_simulation_get_total_hours( simulation ) / simulation->config.hoursToDay; }

static inline uint world_simulation_get_current_second( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_seconds( simulation ) - ( world_simulation_get_total_minutes( simulation ) / simulation->config.secondsToMinute ) ) % simulation->config.secondsToMinute;
}

static inline uint world_simulation_get_current_minute( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_minutes( simulation ) - ( world_simulation_get_total_hours( simulation ) / simulation->config.minutesToHour ) ) % simulation->config.minutesToHour;
}

static inline uint world_simulation_get_current_hour( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_hours( simulation ) - ( world_simulation_get_total_days( simulation ) / simulation->config.hoursToDay ) ) % simulation->config.hoursToDay;
}

/// This returns the total number of seconds for the current day.
static inline uint world_simulation_get_seconds_in_day( const WorldSimulation *simulation )
{
	return ( world_simulation_get_total_seconds( simulation ) ) % world_simulation_get_seconds_to_day( simulation );
}

static inline WorldSimulationTime world_simulation_get_time_of_day( const WorldSimulation *simulation )
{
	uint hour = world_simulation_get_current_hour( simulation );
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

static inline void world_simulation_tick( WorldSimulation *simulation )
{
	simulation->seconds += simulation->speedMultiplier;
}
