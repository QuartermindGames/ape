/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"
#include "ape_scheduler.h"

static PLLinkedList *scheduleList = NULL;

typedef struct SchTask
{
	double            delay;
	char              desc[ 32 ];
	void             *userData;
	ApeSchedulerCallback callback;
	PLLinkedListNode *node;
} SchTask;

static void Cmd_FlushTasks( unsigned int argc, char **argv )
{
	apeFlushTasks();
}

static void Cmd_IsTaskRunning( unsigned int argc, char **argv )
{
	if ( argc <= 1 )
		return;

	PRINT( "%s\n", apeIsScheduledTaskRunning( argv[ 1 ] ) ? "true" : "false" );
}

static void Cmd_KillTask( unsigned int argc, char **argv )
{
	if ( argc <= 1 )
		return;

	apeKillScheduledTask( argv[ 1 ] );
}

static void Cmd_SetTaskDelay( unsigned int argc, char **argv )
{
	if ( argc <= 2 )
		return;

	double delay = strtod( argv[ 2 ], NULL );
	apeSetScheduledTaskDelay( argv[ 1 ], delay );
}

void apeInitializeScheduler( void )
{
	PRINT( "Initializing scheduler\n" );

	PlRegisterConsoleCommand( "sch/flushtasks", "Flush all running tasks.", 0, Cmd_FlushTasks );
	PlRegisterConsoleCommand( "sch/istaskrunning", "Displays 'true' if the specified task is running.", 1, Cmd_IsTaskRunning );
	PlRegisterConsoleCommand( "sch/killtask", "Kill the specified task.", 1, Cmd_KillTask );
	PlRegisterConsoleCommand( "sch/settaskdelay", "Set the delay for the specified task.", 1, Cmd_SetTaskDelay );

	scheduleList = PlCreateLinkedList();
	if ( scheduleList == NULL )
		PRINT_ERROR( "Failed to create schedule linked list!\nPL: %s\n", PlGetError() );
}

void apeShutdownScheduler( void )
{
	PRINT( "Shutting down scheduler\n" );

	PlDestroyLinkedList( scheduleList );
}

unsigned int apeGetNumScheduledTasks( void )
{
	return PlGetNumLinkedListNodes( scheduleList );
}

const char *apeGetScheduledTaskDescription( unsigned int index, double *delay )
{
	if ( scheduleList == NULL )
		return NULL;

	PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < index; ++i )
		{
			node = PlGetNextLinkedListNode( node );
			if ( node == NULL )
				break;
		}
	}

	if ( node == NULL )
		return NULL;

	const SchTask *task = PlGetLinkedListNodeUserData( node );

	if ( delay != NULL )
		*delay = task->delay;

	return task->desc;
}

bool apeIsScheduledTaskRunning( const char *desc )
{
	if ( scheduleList == NULL )
		return false;

	PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	while ( node != NULL )
	{
		SchTask *task = PlGetLinkedListNodeUserData( node );
		if ( strcmp( task->desc, desc ) == 0 )
			return true;

		node = PlGetNextLinkedListNode( node );
	}

	return false;
}

void apePushScheduledTask( const char *desc, ApeSchedulerCallback callback, void *userData, double delay )
{
	SchTask *task = PlMAlloc( sizeof( SchTask ), true );
	snprintf( task->desc, sizeof( task->desc ), "%s", desc );
	task->delay = delay + apeGetNumTicks();
	task->callback = callback;
	task->userData = userData;
	task->node = PlInsertLinkedListNode( scheduleList, task );
}

void apeTickTasks( void )
{
	if ( scheduleList == NULL )
		return;

	PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	while ( node != NULL )
	{
		PLLinkedListNode *nextNode = PlGetNextLinkedListNode( node );
		SchTask          *task = PlGetLinkedListNodeUserData( node );
		if ( task->delay < apeGetNumTicks() )
		{
			PlDestroyLinkedListNode( node );
			task->callback( task->userData, ( task->delay - apeGetNumTicks() ) + 1 );
			task->delay = 0.0;
			PlFree( task );
		}

		node = nextNode;
	}
}

void apeFlushTasks( void )
{
	unsigned int numTasks = PlGetNumLinkedListNodes( scheduleList );
	PlDestroyLinkedListNodes( scheduleList );
	PRINT( "Flushed " PL_FMT_uint32 " tasks\n", numTasks );
}

void apePrintPendingTasks( void )
{
	unsigned int      i = 0;
	PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	while ( node != NULL )
	{
		SchTask *task = PlGetLinkedListNodeUserData( node );
		PRINT( " (%d) %s %f\n", i++, task->desc, task->delay - apeGetNumTicks() );
		node = PlGetNextLinkedListNode( node );
	}
	PRINT( "%d scheduled tasks pending\n", i );
}

static SchTask *GetTaskByDescription( const char *desc )
{
	PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	while ( node != NULL )
	{
		SchTask *task = PlGetLinkedListNodeUserData( node );
		if ( strcmp( task->desc, desc ) == 0 )
			return task;

		node = PlGetNextLinkedListNode( node );
	}
	return NULL;
}

void apeKillScheduledTask( const char *desc )
{
	SchTask *task = GetTaskByDescription( desc );
	if ( task == NULL )
		return;

	PlDestroyLinkedListNode( task->node );
	PlFree( task );
}

void apeSetScheduledTaskDelay( const char *desc, double delay )
{
	SchTask *task = GetTaskByDescription( desc );
	if ( task == NULL )
		return;

	task->delay = delay + apeGetNumTicks();
}
