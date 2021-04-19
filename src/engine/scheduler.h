/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

/****************************************
 * SCHEDULER
 ****************************************/

typedef void (*SchedulerCallback)( void *userData, double delta );
unsigned int Sch_GetNumTasks( void );
const char *Sch_GetTaskDescription( unsigned int index, double *delay );
bool Sch_IsTaskRunning( const char *desc );
void Sch_PushTask( const char *desc, SchedulerCallback callback, void *userData, double delay );
void Sch_RunTasks( void );
void Sch_FlushTasks( void );
void Sch_PrintPendingTasks( void );
