// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/////////////////////////////////////////////////////////////////////////////////////
// World Environment State
/////////////////////////////////////////////////////////////////////////////////////

typedef struct GameWorldSimulationConfig
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
} GameWorldSimulationConfig;

typedef enum GameWorldSimulationTime
{
	WORLD_SIMULATION_TIME_DAWN,
	WORLD_SIMULATION_TIME_MORNING,
	WORLD_SIMULATION_TIME_AFTERNOON,
	WORLD_SIMULATION_TIME_EVENING,
	WORLD_SIMULATION_TIME_NIGHT,
} GameWorldSimulationTime;

typedef struct GameWorldSimulation
{
	GameWorldSimulationConfig config;

	unsigned int seconds;// not *real* seconds!
	unsigned int speedMultiplier;
} GameWorldSimulation;

static void game_world_simulation_initialize( GameWorldSimulation *simulation )
{
	// setup some defaults...
	simulation->config.nightHour = 17;

	simulation->config.secondsToMinute = 60;
	simulation->config.minutesToHour   = 60;
	simulation->config.hoursToDay      = 24;

	simulation->speedMultiplier = 256.0f;
}

static inline unsigned int game_world_simulation_get_seconds_to_hour( const GameWorldSimulation *simulation ) { return simulation->config.secondsToMinute * simulation->config.minutesToHour; }
static inline unsigned int game_world_simulation_get_seconds_to_day( const GameWorldSimulation *simulation ) { return game_world_simulation_get_seconds_to_hour( simulation ) * simulation->config.hoursToDay; }

static inline unsigned int game_world_simulation_get_total_seconds( const GameWorldSimulation *simulation ) { return simulation->seconds; }
static inline unsigned int game_world_simulation_get_total_minutes( const GameWorldSimulation *simulation ) { return simulation->seconds / simulation->config.secondsToMinute; }
static inline unsigned int game_world_simulation_get_total_hours( const GameWorldSimulation *simulation ) { return game_world_simulation_get_total_minutes( simulation ) / simulation->config.minutesToHour; }
static inline unsigned int game_world_simulation_get_total_days( const GameWorldSimulation *simulation ) { return game_world_simulation_get_total_hours( simulation ) / simulation->config.hoursToDay; }

static inline unsigned int game_world_simulation_get_current_second( const GameWorldSimulation *simulation )
{
	return ( game_world_simulation_get_total_seconds( simulation ) - ( game_world_simulation_get_total_minutes( simulation ) / simulation->config.secondsToMinute ) ) % simulation->config.secondsToMinute;
}

static inline unsigned int game_world_simulation_get_current_minute( const GameWorldSimulation *simulation )
{
	return ( game_world_simulation_get_total_minutes( simulation ) - ( game_world_simulation_get_total_hours( simulation ) / simulation->config.minutesToHour ) ) % simulation->config.minutesToHour;
}

static inline unsigned int game_world_simulation_get_current_hour( const GameWorldSimulation *simulation )
{
	return ( game_world_simulation_get_total_hours( simulation ) - ( game_world_simulation_get_total_days( simulation ) / simulation->config.hoursToDay ) ) % simulation->config.hoursToDay;
}

/// This returns the total number of seconds for the current day.
static inline unsigned int game_world_simulation_get_seconds_in_day( const GameWorldSimulation *simulation )
{
	return ( game_world_simulation_get_total_seconds( simulation ) ) % game_world_simulation_get_seconds_to_day( simulation );
}

static inline GameWorldSimulationTime game_world_simulation_get_time_of_day( const GameWorldSimulation *simulation )
{
	unsigned int hour = game_world_simulation_get_current_hour( simulation );
	if ( hour > simulation->config.nightHour ) return WORLD_SIMULATION_TIME_NIGHT;
	if ( hour > simulation->config.eveningHour ) return WORLD_SIMULATION_TIME_EVENING;
	if ( hour > simulation->config.afternoonHour ) return WORLD_SIMULATION_TIME_AFTERNOON;
	if ( hour > simulation->config.morningHour ) return WORLD_SIMULATION_TIME_MORNING;
	if ( hour > simulation->config.dawnHour ) return WORLD_SIMULATION_TIME_DAWN;
	return WORLD_SIMULATION_TIME_NIGHT;
}

static inline const char *game_world_get_time_of_day_descriptor( GameWorldSimulationTime time )
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

static inline void game_world_simulation_tick( GameWorldSimulation *simulation, double delta )
{
	//TODO: hookup delta - we're not using this right now, so I can't test and tweak it, hence why it's not done already!
	simulation->seconds += simulation->speedMultiplier * delta;
}
