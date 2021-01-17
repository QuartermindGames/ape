/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"

#include <PL/pl_llist.h>

static PLLinkedList *scheduleList = NULL;

typedef struct SchTask {
	double delay;
	char desc[ 8 ];
	void *userData;
	SchedulerCallback callback;
	PLLinkedListNode *node;
} SchTask;

bool Sch_IsTaskRunning( const char *desc ) {
	if ( scheduleList == NULL ) {
		return false;
	}

    PLLinkedListNode *node = plGetRootNode( scheduleList );
    while ( node != NULL ) {
        SchTask *task = plGetLinkedListNodeUserData( node );
		if ( strcmp( task->desc, desc ) == 0 ) {
			return true;
		}
        node = plGetNextLinkedListNode( node );
    }

	return false;
}

void Sch_PushTask( const char *desc, SchedulerCallback callback, void *userData, double delay ) {
	if ( scheduleList == NULL ) {
		scheduleList = plCreateLinkedList();
	}

	SchTask *task = Sys_malloc( sizeof( SchTask ) );
	snprintf( task->desc, sizeof( task->desc ), "%s", desc );
	task->delay = delay + Engine_GetNumTicks();
	task->callback = callback;
	task->userData = userData;
	task->node = plInsertLinkedListNode( scheduleList, task );
}

void Sch_RunTasks( void ) {
	if ( scheduleList == NULL ) {
		return;
	}

	PLLinkedListNode *node = plGetRootNode( scheduleList );
	while ( node != NULL ) {
		PLLinkedListNode *nextNode = plGetNextLinkedListNode( node );
		SchTask *task = plGetLinkedListNodeUserData( node );
		if ( task->delay < Engine_GetNumTicks() ) {
			plDestroyLinkedListNode( scheduleList, node );
			task->callback( task->userData, 0.0 );
			free( task );
		}

		node = nextNode;
	}
}

void Sch_FlushTasks( void ) {
	PrintMsg( "Flushing scheduled tasks...\n" );
	plDestroyLinkedList( scheduleList );
	scheduleList = NULL;
}

void Sch_PrintPendingTasks( void ) {
	if ( scheduleList == NULL ) {
		return;
	}

	unsigned int i = 0;
	PLLinkedListNode *node = plGetRootNode( scheduleList );
	while ( node != NULL ) {
		SchTask *task = plGetLinkedListNodeUserData( node );
		PrintMsg( " (%d) %s %f\n", i++, task->desc, task->delay - Engine_GetNumTicks() );
		node = plGetNextLinkedListNode( node );
	}
	PrintMsg( "%d scheduled tasks pending\n", i );
}
