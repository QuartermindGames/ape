/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_linkedlist.h>

#include "yin.h"
#include "scheduler.h"

static PLLinkedList *scheduleList = NULL;

typedef struct SchTask {
	double delay;
	char desc[ 32 ];
	void *userData;
	SchedulerCallback callback;
	PLLinkedListNode *node;
} SchTask;

static void Cmd_FlushTasks( unsigned int argc, char **argv ) {
	u_unused( argc );
	u_unused( argv );
	Sch_FlushTasks();
}

static void Cmd_IsTaskRunning( unsigned int argc, char **argv ) {
	if ( argc <= 1 ) {
		return;
	}

	Print( "%s\n", Sch_IsTaskRunning( argv[ 1 ] ) ? "true" : "false" );
}

static void Cmd_KillTask( unsigned int argc, char **argv ) {
	if ( argc <= 1 ) {
		return;
	}

	Sch_KillTask( argv[ 1 ] );
}

static void Cmd_SetTaskDelay( unsigned int argc, char **argv ) {
	if ( argc <= 2 ) {
		return;
	}

	double delay = strtod( argv[ 2 ], NULL );
	Sch_SetTaskDelay( argv[ 1 ], delay );
}

void Sch_Initialize( void ) {
	Print( "Initializing scheduler\n" );

    PlRegisterConsoleCommand( "Sch.FlushTasks", Cmd_FlushTasks, "Flush all running tasks." );
	PlRegisterConsoleCommand( "Sch.IsTaskRunning", Cmd_IsTaskRunning, "Displays 'true' if the specified task is running." );
	PlRegisterConsoleCommand( "Sch.KillTask", Cmd_KillTask, "Kill the specified task." );
	PlRegisterConsoleCommand( "Sch.SetTaskDelay", Cmd_SetTaskDelay, "Set the delay for the specified task." );
}

unsigned int Sch_GetNumTasks( void ) {
	return PlGetNumLinkedListNodes( scheduleList );
}

const char *Sch_GetTaskDescription( unsigned int index, double *delay ) {
	if ( scheduleList == NULL ) {
		return NULL;
	}

    PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	if ( node != NULL ) {
		for ( unsigned int i = 0; i < index; ++i ) {
			node = PlGetNextLinkedListNode( node );
			if ( node == NULL ) {
				break;
			}
		}
	}

	if ( node == NULL ) {
		return NULL;
	}

    const SchTask *task = PlGetLinkedListNodeUserData( node );

	if ( delay != NULL ) {
		*delay = task->delay;
	}

	return task->desc;
}

bool Sch_IsTaskRunning( const char *desc ) {
	if ( scheduleList == NULL ) {
		return false;
	}

    PLLinkedListNode *node = PlGetFirstNode( scheduleList );
    while ( node != NULL ) {
        SchTask *task = PlGetLinkedListNodeUserData( node );
		if ( strcmp( task->desc, desc ) == 0 ) {
			return true;
		}
        node = PlGetNextLinkedListNode( node );
    }

	return false;
}

void Sch_PushTask( const char *desc, SchedulerCallback callback, void *userData, double delay ) {
	if ( scheduleList == NULL ) {
		scheduleList = PlCreateLinkedList();
	}

	SchTask *task = globalSystem.MAlloc( sizeof( SchTask ), true );
	snprintf( task->desc, sizeof( task->desc ), "%s", desc );
	task->delay = delay + Engine_GetNumTicks();
	task->callback = callback;
	task->userData = userData;
	task->node = PlInsertLinkedListNode( scheduleList, task );
}

void Sch_RunTasks( void ) {
	if ( scheduleList == NULL ) {
		return;
	}

	PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	while ( node != NULL ) {
		PLLinkedListNode *nextNode = PlGetNextLinkedListNode( node );
		SchTask *task = PlGetLinkedListNodeUserData( node );
		if ( task->delay < Engine_GetNumTicks() ) {
			PlDestroyLinkedListNode( scheduleList, node );
			task->callback( task->userData, ( task->delay - Engine_GetNumTicks() ) + 1 );
            task->delay = 0.0;
			globalSystem.Free( task );
		}

		node = nextNode;
	}
}

void Sch_FlushTasks( void ) {
	Print( "Flushing scheduled tasks...\n" );
	PlDestroyLinkedList( scheduleList );
	scheduleList = NULL;
}

void Sch_PrintPendingTasks( void ) {
	if ( scheduleList == NULL ) {
		return;
	}

	unsigned int i = 0;
	PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	while ( node != NULL ) {
		SchTask *task = PlGetLinkedListNodeUserData( node );
		Print( " (%d) %s %f\n", i++, task->desc, task->delay - Engine_GetNumTicks() );
		node = PlGetNextLinkedListNode( node );
	}
	Print( "%d scheduled tasks pending\n", i );
}

static SchTask *GetTaskByDescription( const char *desc ) {
    PLLinkedListNode *node = PlGetFirstNode( scheduleList );
    while ( node != NULL ) {
        SchTask *task = PlGetLinkedListNodeUserData( node );
        if ( strcmp( task->desc, desc ) == 0 ) {
            return task;
        }
        node = PlGetNextLinkedListNode( node );
    }

    Print( "Failed to find specified task, \"%s\"!\n", desc );
	return NULL;
}

void Sch_KillTask( const char *desc ) {
	SchTask *task = GetTaskByDescription( desc );
	if ( task == NULL ) {
		return;
	}

    PlDestroyLinkedListNode( scheduleList, task->node );
    globalSystem.Free( task );
}

void Sch_SetTaskDelay( const char *desc, double delay ) {
	SchTask *task = GetTaskByDescription( desc );
	if ( task == NULL ) {
		return;
	}

    task->delay = delay + Engine_GetNumTicks();
}
