// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "ape_private.h"
#include "ape_scheduler.h"

static QmOsLinkedList *scheduleList;

typedef struct SchTask
{
	double               delay;
	char                 desc[ 32 ];
	void                *userData;
	ApeSchedulerCallback callback;
	QmOsLinkedListNode  *node;
} SchTask;

void ape_scheduler_initialize_( void )
{
	ape_console_print_( "Initializing scheduler\n" );

	scheduleList = qm_os_linked_list_create();
	if ( scheduleList == nullptr )
	{
		ape_console_error_( true, "Failed to create schedule linked list!\nPL: %s\n", PlGetError() );
	}
}

void ape_scheduler_shutdown_( void )
{
	ape_console_print_( "Shutting down scheduler\n" );

	ape_scheduler_flush_();

	qm_os_memory_free( scheduleList );
}

unsigned int ape_scheduler_get_num_tasks_( void )
{
	return qm_os_linked_list_get_size( scheduleList );
}

const char *ape_scheduler_get_task_desc_( unsigned int index, double *delay )
{
	if ( scheduleList == nullptr )
		return nullptr;

	QmOsLinkedListNode *node = qm_os_linked_list_get_front( scheduleList );
	if ( node != nullptr )
	{
		for ( unsigned int i = 0; i < index; ++i )
		{
			node = qm_os_linked_list_node_get_next( node );
			if ( node == nullptr )
				break;
		}
	}

	if ( node == nullptr )
		return nullptr;

	const SchTask *task = qm_os_linked_list_node_get_data( node );

	if ( delay != nullptr )
		*delay = task->delay;

	return task->desc;
}

void ape_scheduler_push_task_( const char *desc, const ApeSchedulerCallback callback, void *userData, double delay )
{
	SchTask *task = QM_OS_MEMORY_NEW( SchTask );
	snprintf( task->desc, sizeof( task->desc ), "%s", desc );
	task->delay    = delay + ape_get_num_ticks();
	task->callback = callback;
	task->userData = userData;
	task->node     = qm_os_linked_list_push_back( scheduleList, task );
}

void ape_scheduler_tick_( void )
{
	if ( scheduleList == nullptr )
		return;

	SchTask *task;
	QM_OS_LINKED_LIST_ITERATE( task, scheduleList, i )
	{
		if ( task->delay < ape_get_num_ticks() )
		{
			task->callback( task->userData, task->delay - ape_get_num_ticks() + 1 );
			task->delay = 0.0;

			qm_os_memory_free( task->node );
			qm_os_memory_free( task );
		}
	}
}

void ape_scheduler_flush_( void )
{
	unsigned int numTasks = qm_os_linked_list_get_size( scheduleList );

	SchTask *task;
	QM_OS_LINKED_LIST_ITERATE( task, scheduleList, i )
	{
		qm_os_memory_free( task->node );
		qm_os_memory_free( task );
	}

	ape_console_print_( "Flushed %u tasks\n", numTasks );
}
