/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>

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

    PLLinkedListNode *child = plGetFirstNode( mmReferenceList );
	while( child != NULL ) {
        PLLinkedListNode *nextChild = plGetNextLinkedListNode( child );
		MemRefCnt *refCnt = plGetLinkedListNodeUserData( child );
		if ( refCnt->numRefs <= 0 && refCnt->ttl < Engine_GetNumTicks() ) {
            refCnt->cleanupFunction( refCnt->userData );
            plDestroyLinkedListNode( mmReferenceList, refCnt->node );
			free( refCnt );
		}
		child = nextChild;
	}

	Sch_PushTask( "mem_cleanup", Mem_CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

void Mem_Initialize( void ) {
	mmReferenceList = plCreateLinkedList();
	if ( mmReferenceList == NULL ) {
		PrintError( "Failed to create memory manager linked list!\n" );
	}

	Sch_PushTask( "mem_cleanup", Mem_CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

MemRefCnt *MemRefCnt_Setup( MemRefCnt_CleanupFunction cleanupFunction, void *userData ) {
    MemRefCnt *ref = globalSystem.MAlloc( sizeof( MemRefCnt ), true );
    ref->userData = userData;
    ref->cleanupFunction = cleanupFunction;
    return ref;
}

void MemRefCnt_AddReference( MemRefCnt *v ) {
    v->numRefs++;
}

void MemRefCnt_RemoveReference( MemRefCnt *v ) {
    assert( v->numRefs > 0 );
    v->numRefs--;
    if ( v->numRefs <= 0 ) {
        v->ttl = ( Engine_GetNumTicks() + 1024 );
    }
}

int MemRefCnt_GetNumOfReferences( const MemRefCnt *v ) {
	return v->numRefs;
}
