// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_hashtable.h>
#include <plcore/pl_timer.h>

#include "common_private.h"

/****************************************
 * PRIVATE
 ****************************************/

#define NUM_SAMPLES 32

typedef struct ComProfilingGroup
{
	const char *key;
	const char *description;

	double        startTime;
	double        timeTaken, oldTimeTaken;
	double        results[ NUM_SAMPLES ];
	unsigned char resultsPos;

	PLHashTableNode *node;
} ComProfilingGroup;
static PLHashTable *profilingGroups = NULL;

/****************************************
 * PUBLIC
 ****************************************/

ComProfilingGroup *com_profiler_get_group( const char *key )
{
	if ( profilingGroups == NULL )
	{
		return NULL;
	}

	ComProfilingGroup *group = ( ComProfilingGroup * ) PlLookupHashTableUserData( profilingGroups, key, strlen( key ) );
	if ( group != NULL )
	{
		// already registered
		return group;
	}

	return NULL;
}

static ComProfilingGroup *RegisterProfilerGroup( const char *key, const char *description )
{
	ComProfilingGroup *group = com_profiler_get_group( key );
	if ( group != NULL )
	{
		return group;
	}

	group              = PL_NEW( ComProfilingGroup );
	group->key         = key;
	group->description = description;

	if ( profilingGroups == NULL )
	{
		profilingGroups = PlCreateHashTable();
		assert( profilingGroups != NULL );
		if ( profilingGroups == NULL )
		{
			return NULL;
		}
	}

	group->node = PlInsertHashTableNode( profilingGroups, group->key, strlen( group->key ), group );

	return group;
}

void com_profiler_start( const char *key )
{
	ComProfilingGroup *group = RegisterProfilerGroup( key, NULL );
	assert( group != NULL );
	if ( group == NULL )
	{
		return;
	}

	group->startTime    = PlGetCurrentSeconds() * 1000.0;
	group->oldTimeTaken = group->timeTaken;
	group->timeTaken    = -1.0;
}

void com_profiler_end( const char *key )
{
	ComProfilingGroup *group = RegisterProfilerGroup( key, NULL );
	assert( group != NULL );
	if ( group == NULL )
	{
		return;
	}

	if ( group->timeTaken == -1.0 )
	{
		group->timeTaken = 0.0;
	}

	group->timeTaken += ( PlGetCurrentSeconds() * 1000.0 ) - group->startTime;
}

const char *com_profiler_get_group_name( const ComProfilingGroup *group ) { return group->key; }

ComProfilingGroup *com_profiler_get_first_group( void )
{
	if ( profilingGroups == NULL )
	{
		return NULL;
	}

	PLHashTableNode *node = PlGetFirstHashTableNode( profilingGroups );
	return ( ComProfilingGroup * ) PlGetHashTableNodeUserData( node );
}

ComProfilingGroup *com_profiler_get_next_group( ComProfilingGroup *group )
{
	if ( profilingGroups == NULL )
	{
		return NULL;
	}

	PLHashTableNode *node = PlGetNextHashTableNode( group->node );
	if ( node != NULL )
	{
		return ( ComProfilingGroup * ) PlGetHashTableNodeUserData( node );
	}

	return NULL;
}

double com_profiler_get_time_taken( const ComProfilingGroup *group )
{
	return group->timeTaken == -1.0 ? group->oldTimeTaken : group->timeTaken;
}

double com_profiler_get_time_average( const ComProfilingGroup *group )
{
	double samples = 0.0;
	for ( unsigned int i = 0; i < NUM_SAMPLES; ++i )
	{
		samples += group->results[ i ];
	}

	return ( samples / NUM_SAMPLES );
}

const double *com_profiler_get_samples( const ComProfilingGroup *group, unsigned int *numPoints )
{
	*numPoints = NUM_SAMPLES;
	return group->results;
}

unsigned int com_profiler_get_num_groups( void )
{
	if ( profilingGroups == NULL )
	{
		return 0;
	}

	return PlGetNumHashTableNodes( profilingGroups );
}

void com_profiler_update_samples( void )
{
	if ( profilingGroups == NULL )
	{
		return;
	}

	ComProfilingGroup *group = com_profiler_get_first_group();
	while ( group != NULL )
	{
		// Shuffle the list along
		for ( unsigned int i = 0; i < NUM_SAMPLES - 1; ++i )
		{
			group->results[ i ] = group->results[ i + 1 ];
		}

		group->results[ NUM_SAMPLES - 1 ] = com_profiler_get_time_taken( group );
		group                             = com_profiler_get_next_group( group );
	}
}

#if 0

void apeUpdateProfilerGraphs( void ) {
	static unsigned int refreshTime = 0;
	if ( refreshTime > apeGetNumTicks() ) {
		return;
	}

	for ( uint8_t i = 0; i < MAX_PROFILER_GROUPS; ++i ) {
		// Shuffle the list along
		for ( uint8_t j = 0; j < NUM_SAMPLES - 1; ++j ) {
			timers[ i ].results[ j ] = timers[ i ].results[ j + 1 ];
		}

		timers[ i ].results[ NUM_SAMPLES - 1 ] = ( float ) apeGetProfilerMeasure( i );
	}

	PL_GET_CVAR( "debug/profilerFrequency", profilerFrequency );
	refreshTime += ( profilerFrequency != NULL ) ? profilerFrequency->i_value : 16;
}

#endif
