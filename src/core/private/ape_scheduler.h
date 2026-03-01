// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/****************************************
 * SCHEDULER
 ****************************************/

typedef void ( *ApeSchedulerCallback )( void *userData, double delta );

void         ape_scheduler_initialize_( void );
void         ape_scheduler_shutdown_( void );
unsigned int ape_scheduler_get_num_tasks_( void );
const char  *ape_scheduler_get_task_desc_( unsigned int index, double *delay );
void         ape_scheduler_push_task_( const char *desc, ApeSchedulerCallback callback, void *userData, double delay );
void         ape_scheduler_tick_( void );
void         ape_scheduler_flush_( void );
