// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/****************************************
 * SCHEDULER
 ****************************************/

typedef void ( *ApeSchedulerCallback )( void *userData, double delta );

void ape_initialize_scheduler_( void );
void ape_shutdown_scheduler_( void );
unsigned int apeGetNumScheduledTasks( void );
const char *apeGetScheduledTaskDescription( unsigned int index, double *delay );
bool apeIsScheduledTaskRunning( const char *desc );
void apePushScheduledTask( const char *desc, ApeSchedulerCallback callback, void *userData, double delay );
void ape_tick_tasks_( void );
void ss_acl_flush_tasks_( void );
void apePrintPendingTasks( void );
void apeKillScheduledTask( const char *desc );
void apeSetScheduledTaskDelay( const char *desc, double delay );
