// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/****************************************
 * SCHEDULER
 ****************************************/

typedef unsigned int ( *ApeSchedulerCallback )( void *userData, double delta );

void         ape_scheduler_initialize_();
void         ape_scheduler_shutdown_();
unsigned int ape_scheduler_get_num_tasks_();
const char  *ape_scheduler_get_task_desc_( unsigned int index, uint64_t *dstNextTick );
void         ape_scheduler_push_task_( const char *desc, ApeSchedulerCallback callback, void *userData, uint64_t delay );
void         ape_scheduler_tick_();
void         ape_scheduler_flush_();
