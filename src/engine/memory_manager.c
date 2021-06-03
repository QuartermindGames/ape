/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_linkedlist.h>

#include "yin.h"

static PLLinkedList *mmReferenceList;

#define MEM_CLEANUP_DELAY 100.0

#define DEBUG_MEMORY 1

void Mem_CleanupCallback( void *unused0, double unused1 )
{
	u_unused( unused0 );
	u_unused( unused1 );

#if defined( DEBUG_MEMORY )
	DebugMsg( "Cleaning up unreferenced resources...\n" );
#endif

	PLLinkedListNode *child = PlGetFirstNode( mmReferenceList );
	while ( child != NULL )
	{
		PLLinkedListNode *nextChild = PlGetNextLinkedListNode( child );
		MemRefCnt *       refCnt    = PlGetLinkedListNodeUserData( child );
		if ( refCnt->numRefs <= 0 && refCnt->ttl < Engine_GetNumTicks() )
		{
			refCnt->cleanupFunction( refCnt->userData );
			PlDestroyLinkedListNode( mmReferenceList, refCnt->node );
			globalSystem.Free( refCnt );
		}
		child = nextChild;
	}

	Sch_PushTask( "mem_cleanup", Mem_CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

void Mem_Initialize( void )
{
	Print( "Initializing memory manager\n" );

	mmReferenceList = PlCreateLinkedList();
	if ( mmReferenceList == NULL )
	{
		PrintError( "Failed to create memory manager linked list!\n" );
	}

	Sch_PushTask( "mem_cleanup", Mem_CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

void Mem_Shutdown( void )
{
	Mem_ForceMemoryFlush();

	unsigned int danglingReferences = PlGetNumLinkedListNodes( mmReferenceList );
	if ( danglingReferences > 0 )
		PrintWarn( "Shutting down memory manager with %u danging references!\n", danglingReferences );
}

MemRefCnt *Mem_SetupReferenceInstance( MemRefCnt *memHandle, MemRefCnt_CleanupFunction cleanupFunction, void *userData )
{
	memHandle->userData        = userData;
	memHandle->cleanupFunction = cleanupFunction;
	PlInsertLinkedListNode( mmReferenceList, memHandle );
	return memHandle;
}

void Mem_AddReference( MemRefCnt *v )
{
#if defined( DEBUG_MEMORY )
	DebugMsg( "Adding reference: description(%s) numRefs(%d) ttl(%u)\n",
	          v->description[ 0 ] == '\0' ? "unknown" : v->description,
	          v->numRefs,
	          v->ttl );
#endif

	v->numRefs++;
}

void Mem_ReleaseReference( MemRefCnt *v )
{
	assert( v->numRefs > 0 );

#if defined( DEBUG_MEMORY )
	DebugMsg( "Releasing reference: description(%s) numRefs(%d) ttl(%u)\n",
	          v->description[ 0 ] == '\0' ? "unknown" : v->description,
	          v->numRefs,
	          v->ttl );
#endif

	v->numRefs--;
	if ( v->numRefs <= 0 )
		v->ttl = ( Engine_GetNumTicks() + 1024 );
}

int Mem_GetNumberOfReferences( const MemRefCnt *v )
{
	return v->numRefs;
}

/**
 * Force call to cleanup callback.
 */
void Mem_ForceMemoryFlush( void )
{
#if defined( DEBUG_MEMORY )
	DebugMsg( "Forcing memory flush of %d objects\n", PlGetNumLinkedListNodes( mmReferenceList ) );
#endif

	Mem_CleanupCallback( NULL, 0.0 );
}
