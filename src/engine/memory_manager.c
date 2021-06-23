/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plcore/pl_linkedlist.h>

#include "yin.h"

static PLLinkedList *mmReferenceList;

#define MEM_CLEANUP_DELAY 200.0

#define DEBUG_MEMORY

static void Mem_Cleanup( bool force )
{
#if defined( DEBUG_MEMORY )
	DebugMsg( "Cleaning up unreferenced resources...\n" );
#endif

	PLLinkedListNode *child = PlGetFirstNode( mmReferenceList );
	while ( child != NULL )
	{
		PLLinkedListNode *nextChild = PlGetNextLinkedListNode( child );
		MemRefCnt *       refCnt    = PlGetLinkedListNodeUserData( child );

#if defined( DEBUG_MEMORY )
		DebugMsg( "%s, numRefs = %d, ttl = %u\n",
		          refCnt->description[ 0 ] == '\0' ? "unknown" : refCnt->description,
		          refCnt->numRefs,
		          refCnt->ttl );
#endif

		if ( refCnt->numRefs <= 0 && ( force || refCnt->ttl < Engine_GetNumTicks() ) )
		{
			//printf( " %p\n", child );
			refCnt->cleanupFunction( refCnt->userData );
			PlDestroyLinkedListNode( mmReferenceList, refCnt->node );
			globalSystem.Free( refCnt );
		}
		child = nextChild;
	}
}

static void Mem_CB_Cleanup( void *unused0, double unused1 )
{
	u_unused( unused0 );
	u_unused( unused1 );

	Mem_Cleanup( false );

	Sch_PushTask( "mem_cleanup", Mem_CB_Cleanup, NULL, MEM_CLEANUP_DELAY );
}

void Mem_Initialize( void )
{
	Print( "Initializing memory manager\n" );

	mmReferenceList = PlCreateLinkedList();
	if ( mmReferenceList == NULL )
		PrintError( "Failed to create memory manager linked list!\n" );

	Sch_PushTask( "mem_cleanup", Mem_CB_Cleanup, NULL, MEM_CLEANUP_DELAY );
}

void Mem_Shutdown( void )
{
	unsigned int danglingReferences = PlGetNumLinkedListNodes( mmReferenceList );
	while ( danglingReferences > 0 )
	{
		Mem_Cleanup( true );

		unsigned int n = PlGetNumLinkedListNodes( mmReferenceList );
		if ( n == danglingReferences )
			break;
	}

	danglingReferences = PlGetNumLinkedListNodes( mmReferenceList );
	if ( danglingReferences > 0 )
		PrintWarn( "Shutting down memory manager with %u dangling references!\n", danglingReferences );
}

MemRefCnt *Mem_SetupReferenceInstance( const char *description, MemRefCnt *memHandle, MemRefCnt_CleanupFunction cleanupFunction, void *userData )
{
	snprintf( memHandle->description, sizeof( memHandle->description ), "%s", description );

	memHandle->userData        = userData;
	memHandle->cleanupFunction = cleanupFunction;

	PlInsertLinkedListNode( mmReferenceList, memHandle );
	return memHandle;
}

void Mem_AddReference( MemRefCnt *v )
{
	v->numRefs++;
#if defined( DEBUG_MEMORY )
	DebugMsg( "Adding reference: description(%s) numRefs(%d) ttl(%u)\n",
	          v->description[ 0 ] == '\0' ? "unknown" : v->description,
	          v->numRefs,
	          v->ttl );
#endif
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
