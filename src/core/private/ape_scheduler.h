/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

/****************************************
 * SCHEDULER
 ****************************************/

typedef void ( *ApeSchedulerCallback )( void *userData, double delta );

void apeInitializeScheduler( void );
void apeShutdownScheduler( void );
unsigned int apeGetNumScheduledTasks( void );
const char *apeGetScheduledTaskDescription( unsigned int index, double *delay );
bool apeIsScheduledTaskRunning( const char *desc );
void apePushScheduledTask( const char *desc, ApeSchedulerCallback callback, void *userData, double delay );
void apeTickTasks( void );
void apeFlushTasks( void );
void apePrintPendingTasks( void );
void apeKillScheduledTask( const char *desc );
void apeSetScheduledTaskDelay( const char *desc, double delay );
