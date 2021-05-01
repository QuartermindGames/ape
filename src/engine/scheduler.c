/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_linkedlist.h>

#include "yin.h"

static PLLinkedList *scheduleList = NULL;

typedef struct SchTask {
	double delay;
	char desc[ 8 ];
	void *userData;
	SchedulerCallback callback;
	PLLinkedListNode *node;
} SchTask;

unsigned int Sch_GetNumTasks( void ) {
	return PlGetNumLinkedListNodes( scheduleList );
}

const char *Sch_GetTaskDescription( unsigned int index, double *delay ) {
	if ( scheduleList == NULL ) {
		return NULL;
	}

    PLLinkedListNode *node = PlGetFirstNode( scheduleList );
	for ( unsigned int i = 0; i < index; ++i ) {
		node = PlGetNextLinkedListNode( node );
		if ( node == NULL ) {
			return NULL;
		}
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
			task->callback( task->userData, 0.0 );
			free( task );
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
