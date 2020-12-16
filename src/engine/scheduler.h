/* Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

/****************************************
 * SCHEDULER
 ****************************************/

typedef void (*SchedulerCallback)( void *userData, double delta );
void Sch_PushTask( const char *desc, SchedulerCallback callback, void *userData, double delay );
void Sch_RunTasks( void );
void Sch_FlushTasks( void );
void Sch_PrintPendingTasks( void );
