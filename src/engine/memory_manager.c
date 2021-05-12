/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_linkedlist.h>

#include "yin.h"

typedef struct MemRefCnt {
    int numRefs;        /* number of total references */
    unsigned int ttl;   /* time to live */
    void *userData;
    MemRefCnt_CleanupFunction cleanupFunction;
	PLLinkedListNode *node;
} MemRefCnt;

static PLLinkedList *mmReferenceList;

#define MEM_CLEANUP_DELAY 100.0

void Mem_CleanupCallback( void *userData, double delta ) {
	DebugMsg( "Cleaning up unreferenced resources...\n" );

    PLLinkedListNode *child = PlGetFirstNode( mmReferenceList );
	while( child != NULL ) {
        PLLinkedListNode *nextChild = PlGetNextLinkedListNode( child );
		MemRefCnt *refCnt = PlGetLinkedListNodeUserData( child );
		if ( refCnt->numRefs <= 0 && refCnt->ttl < Engine_GetNumTicks() ) {
            refCnt->cleanupFunction( refCnt->userData );
            PlDestroyLinkedListNode( mmReferenceList, refCnt->node );
			globalSystem.Free( refCnt );
		}
		child = nextChild;
	}

	Sch_PushTask( "mem_cleanup", Mem_CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

void Mem_Initialize( void ) {
	Print( "Initializing memory manager\n" );

	mmReferenceList = PlCreateLinkedList();
	if ( mmReferenceList == NULL ) {
		PrintError( "Failed to create memory manager linked list!\n" );
	}

	Sch_PushTask( "mem_cleanup", Mem_CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

void Mem_Shutdown( void ) {

}

MemRefCnt *Mem_SetupReferenceInstance( MemRefCnt_CleanupFunction cleanupFunction, void *userData ) {
    MemRefCnt *ref = globalSystem.MAlloc( sizeof( MemRefCnt ), true );
    ref->userData = userData;
    ref->cleanupFunction = cleanupFunction;
    return ref;
}

void Mem_AddReference( MemRefCnt *v ) {
    v->numRefs++;
}

void Mem_ReleaseReference( MemRefCnt *v ) {
    assert( v->numRefs > 0 );
    v->numRefs--;
    if ( v->numRefs <= 0 ) {
        v->ttl = ( Engine_GetNumTicks() + 1024 );
    }
}

int Mem_GetNumberOfReferences( const MemRefCnt *v ) {
	return v->numRefs;
}
