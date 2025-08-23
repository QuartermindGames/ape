// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "qmos/public/qm_os_time.h"

#include <plcore/pl_hashtable.h>
#include <plcore/pl_timer.h>

#include "common_private.h"

#define NUM_SAMPLES 32

typedef struct ComProfilingGroup
{
	const char *key;

	double        startTime;
	double        timeTaken, lastTimeTaken;
	double        results[ NUM_SAMPLES ];
	unsigned char resultsPos;

	PLHashTableNode *node;
} ComProfilingGroup;
static PLHashTable *profilingGroups = nullptr;

ComProfilingGroup *com_profiler_get_group( const char *key )
{
	if ( profilingGroups == nullptr )
	{
		return nullptr;
	}

	ComProfilingGroup *group = PlLookupHashTableUserData( profilingGroups, key, strlen( key ) );
	if ( group != nullptr )
	{
		// already registered
		return group;
	}

	return nullptr;
}

static ComProfilingGroup *register_profiler_group( const char *key )
{
	ComProfilingGroup *group = com_profiler_get_group( key );
	if ( group != nullptr )
	{
		return group;
	}

	group      = PL_NEW( ComProfilingGroup );
	group->key = key;

	if ( profilingGroups == nullptr )
	{
		profilingGroups = PlCreateHashTable();
		if ( profilingGroups == nullptr )
		{
			return nullptr;
		}
	}

	group->node = PlInsertHashTableNode( profilingGroups, group->key, strlen( group->key ), group );

	return group;
}

bool com_profiler_start( const char *key )
{
	ComProfilingGroup *group = register_profiler_group( key );
	if ( group == nullptr )
	{
		return false;
	}

	assert( group->startTime == 0.0 );

	group->startTime = qm_os_time_get_seconds() * 1000.0;

	return true;
}

bool com_profiler_end( const char *key )
{
	ComProfilingGroup *group = register_profiler_group( key );
	if ( group == nullptr )
	{
		return false;
	}

	assert( group->startTime != 0.0 );

	group->lastTimeTaken = group->timeTaken;
	group->timeTaken += ( qm_os_time_get_seconds() * 1000.0 ) - group->startTime;
	group->startTime = 0.0;

	return true;
}

const char *com_profiler_get_group_name( const ComProfilingGroup *group ) { return group->key; }

ComProfilingGroup *com_profiler_get_first_group( void )
{
	if ( profilingGroups == nullptr )
	{
		return nullptr;
	}

	PLHashTableNode *node = PlGetFirstHashTableNode( profilingGroups );
	return PlGetHashTableNodeUserData( node );
}

ComProfilingGroup *com_profiler_get_next_group( const ComProfilingGroup *group )
{
	if ( profilingGroups == nullptr )
	{
		return nullptr;
	}

	PLHashTableNode *node = PlGetNextHashTableNode( group->node );
	if ( node != nullptr )
	{
		return PlGetHashTableNodeUserData( node );
	}

	return nullptr;
}

double com_profiler_get_time_average( const ComProfilingGroup *group )
{
	double samples = 0.0;
	for ( unsigned int i = 0; i < NUM_SAMPLES; ++i )
	{
		samples += group->results[ i ];
	}

	return samples / NUM_SAMPLES;
}

const double *com_profiler_get_samples( const ComProfilingGroup *group, unsigned int *numPoints )
{
	*numPoints = NUM_SAMPLES;
	return group->results;
}

unsigned int com_profiler_get_num_groups( void )
{
	if ( profilingGroups == nullptr )
	{
		return 0;
	}

	return PlGetNumHashTableNodes( profilingGroups );
}

void com_profiler_update_samples( void )
{
	if ( profilingGroups == nullptr )
	{
		return;
	}

	PL_GET_CVAR( "debug/profilerFrequency", profilerFrequency );
	static uint16_t nextRefresh = 0;

	ComProfilingGroup *group = com_profiler_get_first_group();
	while ( group != nullptr )
	{
		assert( group->startTime == 0.0 );

		if ( nextRefresh == 0 )
		{
			// Shuffle the list along
			for ( unsigned int i = 0; i < NUM_SAMPLES - 1; ++i )
			{
				group->results[ i ] = group->results[ i + 1 ];
			}

			group->results[ NUM_SAMPLES - 1 ] = group->timeTaken;
		}

		group->timeTaken = 0.0;

		group = com_profiler_get_next_group( group );
	}

	if ( nextRefresh == 0 )
	{
		nextRefresh = ( profilerFrequency != nullptr ? profilerFrequency->i_value : 32 );
		return;
	}

	nextRefresh--;
}
