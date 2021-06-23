/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

void Mem_Initialize( void );
void Mem_Shutdown( void );

typedef void ( *MemRefCnt_CleanupFunction )( void *userData );
typedef struct MemRefCnt
{
	char                      description[ 16 ];// Simple description, for debugging
	int                       numRefs;          // Number of total references
	unsigned int              ttl;              // Time to live
	void *                    userData;         // Pointer to original data struct
	MemRefCnt_CleanupFunction cleanupFunction;  // Function that deals with the *real* cleanup
	struct PLLinkedListNode * node;             // Index into the memory reference list
} MemRefCnt;

MemRefCnt *Mem_SetupReferenceInstance( const char *description, MemRefCnt *memHandle, MemRefCnt_CleanupFunction cleanupFunction, void *userData );

void Mem_AddReference( MemRefCnt *v );
void Mem_ReleaseReference( MemRefCnt *v );

int Mem_GetNumberOfReferences( const MemRefCnt *v );
