// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "ape_private.h"
#include "ape_scheduler.h"

//TODO: this hasn't received a lot of TLC; delay should be measured in ms, rather than ticks

static QmOsLinkedList *scheduleList;

typedef struct ApeSchedulerTask
{
	char                 desc[ 32 ];
	uint64_t             nextTick;
	void                *userData;
	ApeSchedulerCallback callback;
	QmOsLinkedListNode  *node;
} ApeSchedulerTask;

void ape_scheduler_initialize_()
{
	ape_console_print_( "Initializing scheduler\n" );

	scheduleList = qm_os_linked_list_create();
	if ( scheduleList == nullptr )
	{
		ape_console_error_( true, "Failed to create schedule linked list!\nPL: %s\n", PlGetError() );
	}
}

void ape_scheduler_shutdown_()
{
	ape_console_print_( "Shutting down scheduler\n" );

	ape_scheduler_flush_();

	qm_os_memory_free( scheduleList );
}

unsigned int ape_scheduler_get_num_tasks_()
{
	return qm_os_linked_list_get_size( scheduleList );
}

const char *ape_scheduler_get_task_desc_( unsigned int index, uint64_t *dstNextTick )
{
	if ( scheduleList == nullptr )
	{
		return nullptr;
	}

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

	const ApeSchedulerTask *task = qm_os_linked_list_node_get_data( node );
	if ( dstNextTick != nullptr )
	{
		*dstNextTick = task->nextTick;
	}

	return task->desc;
}

void ape_scheduler_push_task_( const char *desc, const ApeSchedulerCallback callback, void *userData, uint64_t delay )
{
	ApeSchedulerTask *task = QM_OS_MEMORY_NEW( ApeSchedulerTask );
	qm_os_string_copy( task->desc, desc, sizeof( task->desc ) );
	task->nextTick = delay + ape_get_num_ticks();
	task->callback = callback;
	task->userData = userData;
	task->node     = qm_os_linked_list_push_back( scheduleList, task );
}

void ape_scheduler_tick_()
{
	if ( scheduleList == nullptr )
	{
		return;
	}

	ApeSchedulerTask *task;
	QM_OS_LINKED_LIST_ITERATE( task, scheduleList, i )
	{
		if ( task->nextTick > ape_get_num_ticks() )
		{
			continue;
		}

		const unsigned int nextTick = task->callback( task->userData, task->nextTick - ape_get_num_ticks() + 1 );
		if ( nextTick == 0 )
		{
			ape_console_print_( "Task %s was automatically terminated\n", task->desc );
			qm_os_memory_free( task->node );
			qm_os_memory_free( task );
			continue;
		}

		task->nextTick = nextTick + ape_get_num_ticks();
	}
}

void ape_scheduler_flush_()
{
	const unsigned int numTasks = qm_os_linked_list_get_size( scheduleList );

	ApeSchedulerTask *task;
	QM_OS_LINKED_LIST_ITERATE( task, scheduleList, i )
	{
		qm_os_memory_free( task->node );
		qm_os_memory_free( task );
	}

	ape_console_print_( "Flushed %u tasks\n", numTasks );
}
