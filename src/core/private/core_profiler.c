// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"

/****************************************
 * PRIVATE
 ****************************************/

#define NUM_GRAPH_POINTS 32

typedef struct ProfilerTimer
{
	char   description[ 64 ];
	double startTime;
	double timeTaken;
	double results[ NUM_GRAPH_POINTS ];
} ProfilerTimer;
static ProfilerTimer timers[ MAX_PROFILER_GROUPS ];

/****************************************
 * PUBLIC
 ****************************************/

const char *cpuProfilerDescriptions[ MAX_PROFILER_GROUPS ];

void ogeInitializeProfiler( void )
{
	PRINT( "Initializing profiler\n" );

	PL_ZERO( timers, sizeof( ProfilerTimer ) * MAX_PROFILER_GROUPS );

	PL_ZERO( cpuProfilerDescriptions, sizeof( const char * ) * MAX_PROFILER_GROUPS );
	cpuProfilerDescriptions[ PROFILE_SIM_ALL ]     = "SIM_ALL";
	cpuProfilerDescriptions[ PROFILE_TICK_CLIENT ] = "TICK_CLIENT";
	cpuProfilerDescriptions[ PROFILE_TICK_SERVER ] = "TICK_SERVER";
	cpuProfilerDescriptions[ PROFILE_DRAW_ALL ]    = "DRAW_ALL";
	cpuProfilerDescriptions[ PROFILE_DRAW_WORLD ]  = "DRAW_WORLD";
	cpuProfilerDescriptions[ PROFILE_DRAW_ACTORS ] = "DRAW_ACTORS";
	cpuProfilerDescriptions[ PROFILE_DRAW_UI ]     = "DRAW_UI";
	cpuProfilerDescriptions[ PROFILE_DRAW_GUI ]    = "DRAW_GUI";
}

void ogeProfiler_EndFrame( void )
{
	// todo: for now we've gone back to resetting this on StartMeasure call
	//for ( unsigned int i = 0; i < MAX_PROFILER_GROUPS; ++i )
	//	timers[ i ].timeTaken = 0;
}

void Profiler_StartMeasure( ProfilerGroup group )
{
	timers[ group ].startTime = PlGetCurrentSeconds() * 1000;
	timers[ group ].timeTaken = 0;
}

void Profiler_EndMeasure( ProfilerGroup group )
{
	// inc. as we have some cases of recursion
	timers[ group ].timeTaken += ( PlGetCurrentSeconds() * 1000 ) - timers[ group ].startTime;
}

double Profiler_GetMeasure( ProfilerGroup group )
{
	return timers[ group ].timeTaken;
}

void ogeProfiler_UpdateGraphs( void )
{
	static unsigned int refreshTime = 0;
	if ( refreshTime > ogeGetNumTicks() )
		return;

	for ( uint8_t i = 0; i < MAX_PROFILER_GROUPS; ++i )
	{
		// Shuffle the list along
		for ( uint8_t j = 0; j < NUM_GRAPH_POINTS - 1; ++j )
			timers[ i ].results[ j ] = timers[ i ].results[ j + 1 ];

		timers[ i ].results[ NUM_GRAPH_POINTS - 1 ] = ( float ) Profiler_GetMeasure( i );
	}

	PL_GET_CVAR( "debug.profilerFrequency", profilerFrequency );
	refreshTime += ( profilerFrequency != NULL ) ? profilerFrequency->i_value : 16;
}

const double *Profiler_GetGraph( ProfilerGroup group, uint8_t *numPoints )
{
	*numPoints = NUM_GRAPH_POINTS;
	return timers[ group ].results;
}

/**
 * Return a single value from the head of the performance graph.
 */
double Profiler_GetGraphValue( ProfilerGroup group )
{
	return timers[ group ].results[ NUM_GRAPH_POINTS - 1 ];
}

double Profiler_GetGraphAverage( ProfilerGroup group )
{
	double t = 0.0;
	for ( unsigned int i = 0; i < NUM_GRAPH_POINTS; ++i )
		t += timers[ group ].results[ i ];

	return ( t / NUM_GRAPH_POINTS );
}
