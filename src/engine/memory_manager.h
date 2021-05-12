/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

void Mem_Initialize( void );
void Mem_Shutdown( void );

typedef struct MemRefCnt MemRefCnt;

typedef void ( *MemRefCnt_CleanupFunction )( void *userData );
MemRefCnt *Mem_SetupReferenceInstance( MemRefCnt_CleanupFunction cleanupFunction, void *userData );

void Mem_AddReference( MemRefCnt *v );
void Mem_ReleaseReference( MemRefCnt *v );

int Mem_GetNumberOfReferences( const MemRefCnt *v );
