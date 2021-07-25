/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plcore/pl_linkedlist.h>

#include "yin.h"

/* ======================================================================
 * Cache Pools
 * ====================================================================*/

static PLLinkedList *memCachePools[ MEM_CACHE_END ];

static void MEM_InitializeCachePools( void )
{
	for ( uint8_t i = 0; i < MEM_CACHE_END; ++i )
	{
		memCachePools[ i ] = PlCreateLinkedList();
		if ( memCachePools[ i ] == NULL )
			PrintError( "Failed to create cache pool: " COM_FMT_int32 "\nPL: %s\n", i, PlGetError() );
	}
}

void MEM_CacheData( const char *id, uint8_t pool, void *data )
{
	/* ensure the data hasn't been cached already */
	void *cachedData = MEM_GetCachedData( id, pool );
	if ( cachedData != NULL )
		PrintError( "Attempted to cache duplicate data: %s\n", id );

	MEMCacheHeader *header = globalSystem.MAlloc( sizeof( MEMCacheHeader ), true );
	snprintf( header->id, sizeof( header->id ), "%s", id );
	header->pool     = pool;
	header->userData = data;

	PLLinkedListNode *node = PlInsertLinkedListNode( memCachePools[ pool ], header );
	if ( node == NULL )
		PrintError( "Failed to insert node for cache pool!\n" );
}

void *MEM_GetCachedData( const char *id, uint8_t pool )
{
	PLLinkedListNode *node = PlGetFirstNode( memCachePools[ pool ] );
	while ( node != NULL )
	{
		MEMCacheHeader *header = PlGetLinkedListNodeUserData( node );
		if ( strncmp( header->id, id, MEM_MAX_ID ) == 0 )
			return header->userData;

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

static void MEM_RemoveCachedData( const char *id, uint8_t pool )
{
	PLLinkedListNode *node = PlGetFirstNode( memCachePools[ pool ] );
	while ( node != NULL )
	{
		MEMCacheHeader *header = PlGetLinkedListNodeUserData( node );
		if ( strncmp( header->id, id, MEM_MAX_ID ) == 0 )
		{
			PlDestroyLinkedListNode( memCachePools[ pool ], node );
			globalSystem.Free( header );
			return;
		}

		node = PlGetNextLinkedListNode( node );
	}

	PrintWarn( "Attempted to remove node from cache pool, but failed: %s\n", id );
}

/* ======================================================================
 * Reference Counting and Garbage Collection
 * ====================================================================*/

static PLLinkedList *mmReferenceList;

#define MEM_CLEANUP_DELAY 200.0

#define DEBUG_MEMORY

static bool MEM_FreeReference( MEMReference *m, bool force )
{
#if defined( DEBUG_MEMORY )
	DebugMsg( "%s, numRefs = %d, ttl = %u\n",
	          m->description[ 0 ] == '\0' ? "unknown" : m->description,
	          m->numRefs,
	          m->ttl );
#endif

	if ( m->numRefs <= 0 && ( force || m->ttl < Engine_GetNumTicks() ) )
	{
		/* remove it from whatever cached list it exists in */
		//MEM_RemoveCachedData( )

        PLLinkedListNode *node = m->node;
		m->cleanupFunction( m->userData );
        PlDestroyLinkedListNode( mmReferenceList, node );
		return true;
	}

	return false;
}

static void CleanupUnreferencedResources( bool force )
{
	PLLinkedListNode *child = PlGetFirstNode( mmReferenceList );
	while ( child != NULL )
	{
		PLLinkedListNode *nextChild = PlGetNextLinkedListNode( child );
		MEMReference *    m         = PlGetLinkedListNodeUserData( child );

#if defined( DEBUG_MEMORY )
        DebugMsg( " %s (" COM_FMT_int32 ")\n", m->description, m->numRefs );
#endif

		MEM_FreeReference( m, force );

		child = nextChild;
	}
}

#define MEM_CLEANUP_TASK_NAME "mem_cleanup"

static void MEM_CB_Cleanup( void *unused0, double unused1 )
{
	u_unused( unused0 );
	u_unused( unused1 );

	CleanupUnreferencedResources( false );

	Sch_PushTask( MEM_CLEANUP_TASK_NAME, MEM_CB_Cleanup, NULL, MEM_CLEANUP_DELAY );
}

void MEM_Initialize( void )
{
	Print( "Initializing memory manager\n" );

	MEM_InitializeCachePools();

	mmReferenceList = PlCreateLinkedList();
	if ( mmReferenceList == NULL )
		PrintError( "Failed to create memory manager linked list!\n" );

	Sch_PushTask( MEM_CLEANUP_TASK_NAME, MEM_CB_Cleanup, NULL, MEM_CLEANUP_DELAY );
}

void MEM_Shutdown( void )
{
	unsigned int danglingReferences = PlGetNumLinkedListNodes( mmReferenceList );
	while ( danglingReferences > 0 )
	{
#if defined( DEBUG_MEMORY )
        DebugMsg( " dangling references: " COM_FMT_uint32 "\n", danglingReferences );
#endif

		CleanupUnreferencedResources( true );

		unsigned int n = PlGetNumLinkedListNodes( mmReferenceList );
		if ( n == danglingReferences )
			break;

		danglingReferences = n;
	}

	danglingReferences = PlGetNumLinkedListNodes( mmReferenceList );
	if ( danglingReferences > 0 )
		PrintWarn( "Shutting down memory manager with %u dangling references!\n", danglingReferences );
}

MEMReference *MEM_SetupReferenceInstance( const char *description, MEMReference *m, MEMReference_CleanupFunction cleanupFunction, void *userData )
{
	snprintf( m->description, sizeof( m->description ), "%s", description );

	m->userData        = userData;
	m->cleanupFunction = cleanupFunction;
	m->isInitialized   = true;
	m->node            = PlInsertLinkedListNode( mmReferenceList, m );

	return m;
}

void MEM_AddReference( MEMReference *m )
{
	m->numRefs++;
#if defined( DEBUG_MEMORY )
	DebugMsg( "Adding reference: description(%s) numRefs(%d) ttl(%u)\n",
	          m->description[ 0 ] == '\0' ? "unknown" : m->description,
	          m->numRefs,
	          m->ttl );
#endif
}

void MEM_ReleaseReference( MEMReference *m )
{
	assert( m->numRefs > 0 );

#if defined( DEBUG_MEMORY )
	DebugMsg( "Releasing reference: description(%s) numRefs(%d) ttl(%u)\n",
	          m->description[ 0 ] == '\0' ? "unknown" : m->description,
	          m->numRefs,
	          m->ttl );
#endif

	m->numRefs--;
	if ( m->numRefs <= 0 )
		m->ttl = ( Engine_GetNumTicks() + 1024 );
}

int MEM_GetNumberOfReferences( const MEMReference *m )
{
	return m->numRefs;
}

/* ======================================================================
 * Temporary Buffer Allocation
 * ====================================================================*/

static void MEM_CB_CleanupTempAlloc( void *userData )
{
	globalSystem.Free( userData );
}

/**
 * Allocates a pool of memory that will be automatically
 * cleaned up.
 */
void *MEM_TempAlloc( MEMReference *m, size_t size )
{
	void *buf = globalSystem.MAlloc( size, true );
	MEM_SetupReferenceInstance( "temp", m, MEM_CB_CleanupTempAlloc, buf );
	return buf;
}

/**
 * Attempts to immediately free the given resource.
 * If this isn't called, resource will be cleaned
 * up automatically later.
 */
void MEM_TempFree( MEMReference *m )
{
	if ( !MEM_FreeReference( m, false ) )
		PrintWarn( "Failed to cleanup temporary pool!\n" );
}
