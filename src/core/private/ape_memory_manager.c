/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"

static PLMemoryGroup *memoryGroups[ APE_MAX_CACHE_POOLS ];

/* ======================================================================
 * Cache Pools
 * ====================================================================*/

static PLLinkedList *memCachePools[ APE_MAX_CACHE_POOLS ];

static void InitializeCachePools( void )
{
	for ( unsigned int i = 0; i < APE_MAX_CACHE_POOLS; ++i )
	{
		memCachePools[ i ] = PlCreateLinkedList();
		if ( memCachePools[ i ] == NULL )
		{
			PRINT_ERROR( "Failed to create cache pool: " PL_FMT_int32 "\nPL: %s\n", i, PlGetError() );
		}
	}
}

void apeAddToCachePool( const char *id, ApeCachePool pool, void *data )
{
	/* ensure the data hasn't been cached already */
	void *cachedData = apeGetCachedData( id, pool );
	if ( cachedData != NULL )
	{
		PRINT_ERROR( "Attempted to cache duplicate data: %s\n", id );
	}

	ApeMemoryCacheHeader *header = PL_NEW( ApeMemoryCacheHeader );
	header->id = PlGenerateHashSDBM( id );
	header->pool = pool;
	header->userData = data;
	snprintf( header->description, sizeof( header->description ), "%s", id );

	PLLinkedListNode *node = PlInsertLinkedListNode( memCachePools[ pool ], header );
	if ( node == NULL )
	{
		PRINT_ERROR( "Failed to insert node for cache pool!\n" );
	}

	PRINT( "Added \"%s\" (%u) to cache pool %u\n", id, header->id, pool );
}

static ApeMemoryCacheHeader *GetCache( uint32_t id, uint8_t pool )
{
	PLLinkedListNode *node = PlGetFirstNode( memCachePools[ pool ] );
	while ( node != NULL )
	{
		ApeMemoryCacheHeader *header = PlGetLinkedListNodeUserData( node );
		if ( header->id == id )
			return header;

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

void *apeGetCachedData( const char *id, ApeCachePool pool )
{
	uint32_t hashedName = PlGenerateHashSDBM( id );
	ApeMemoryCacheHeader *header = GetCache( hashedName, pool );
	if ( header != NULL )
		return header->userData;

	return NULL;
}

static void RemoveFromCache( uint32_t id, uint8_t pool )
{
	ApeMemoryCacheHeader *header = GetCache( id, pool );
	if ( header == NULL )
	{
		PRINT_WARNING( "Attempted to remove node from cache pool, but failed: %s\n", id );
		return;
	}

	PlDestroyLinkedListNode( header->node );

	PRINT( "Removed \"%s\" from cache\n", header->description );

	PL_DELETE( header );
}

/* ======================================================================
 * Reference Counting and Garbage Collection
 * ====================================================================*/

static PLLinkedList *mmReferenceList;

#define MEM_CLEANUP_DELAY 200.0

//#	define DEBUG_MEMORY

static bool FreeReference( ApeMemoryReference *m, bool force )
{
#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "%s, numRefs = %d, ttl = %u\n",
	             m->cache == NULL ? "unknown" : m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#endif

	if ( m->numReferences <= 0 && ( force || m->timeToLive < ape_get_num_ticks() ) )
	{
		/* remove it from whatever cached list it exists in */
		if ( m->cache != NULL )
		{
			RemoveFromCache( m->cache->id, m->cache->pool );
			m->cache = NULL;
		}

		PLLinkedListNode *node = m->node;
		m->cleanupFunction( m->userData );
		PlDestroyLinkedListNode( node );

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
		ApeMemoryReference *m = PlGetLinkedListNodeUserData( child );

#if defined( DEBUG_MEMORY )
		PRINT_DEBUG( " %s (" PL_FMT_int32 ")\n",
		             m->cache == NULL ? "unknown" : m->cache->description,
		             m->numReferences );
#endif

		FreeReference( m, force );

		child = nextChild;
	}
}

#define MEM_CLEANUP_TASK_NAME "mem_cleanup"

static void CleanupCallback( PL_UNUSED void *unused0, PL_UNUSED double unused1 )
{
	CleanupUnreferencedResources( false );

	apePushScheduledTask( MEM_CLEANUP_TASK_NAME, CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

void ape_initialize_memory_manager_( void )
{
	PRINT( "Initializing memory manager\n" );

	for ( unsigned int i = 0; i < APE_MAX_CACHE_POOLS; ++i )
		memoryGroups[ i ] = PlCreateMemoryGroup();

	InitializeCachePools();

	mmReferenceList = PlCreateLinkedList();
	if ( mmReferenceList == NULL )
	{
		PRINT_ERROR( "Failed to create memory manager linked list!\n" );
	}

	apePushScheduledTask( MEM_CLEANUP_TASK_NAME, CleanupCallback, NULL, MEM_CLEANUP_DELAY );
}

void ape_shutdown_memory_manager_( void )
{
	apeFlushUnreferencedResources();

	unsigned int danglingReferences = PlGetNumLinkedListNodes( mmReferenceList );
	if ( danglingReferences > 0 )
	{
		PRINT_WARNING( "Shutting down memory manager with %u dangling references!\n", danglingReferences );
	}

	for ( unsigned int i = 0; i < APE_MAX_CACHE_POOLS; ++i )
	{
		PlDestroyMemoryGroup( memoryGroups[ i ] );
	}
}

unsigned int apeFlushUnreferencedResources( void )
{
	unsigned int references = PlGetNumLinkedListNodes( mmReferenceList );
	while ( references > 0 )
	{
#if defined( DEBUG_MEMORY )
		PRINT_DEBUG( " dangling references: " PL_FMT_uint32 "\n", references );
#endif

		CleanupUnreferencedResources( true );

		unsigned int n = PlGetNumLinkedListNodes( mmReferenceList );
		if ( n == references )
			break;

		references = n;
	}

	return references;
}

ApeMemoryReference *ape_mm_setup_reference( const char *id, uint8_t pool, ApeMemoryReference *m, MMReference_CleanupFunction cleanupFunction, void *userData )
{
	m->cache = apeGetCachedData( id, pool );
	m->userData = userData;
	m->cleanupFunction = cleanupFunction;
	m->isInitialized = true;
	m->node = PlInsertLinkedListNode( mmReferenceList, m );
	return m;
}

void ape_mm_add_reference( ApeMemoryReference *m )
{
	m->numReferences++;
	m->timeToLive = ( ape_get_num_ticks() + 1024 );
#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "Adding reference: description(%s) numRefs(%d) ttl(%u)\n",
	             m->cache == NULL ? "unknown" : m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#endif
}

void ss_acl_mm_release( ApeMemoryReference *m )
{
	assert( m->numReferences > 0 );

#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "Releasing reference: description(%s) numRefs(%d) ttl(%u)\n",
	             m->cache == NULL ? "unknown" : m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#endif

	m->numReferences--;
	if ( m->numReferences <= 0 )
	{
		m->timeToLive = ( ape_get_num_ticks() + 1024 );
	}
}

int apeGetNumberOfReferences( const ApeMemoryReference *m )
{
	return m->numReferences;
}

/* ======================================================================
 * Temporary Buffer Allocation
 * ====================================================================*/

static void CleanupTempAllocCallback( void *userData )
{
	PL_DELETE( userData );
}

/**
 * Allocates a pool of memory that will be automatically
 * cleaned up.
 */
void *apeTempAlloc( ApeMemoryReference *m, size_t size )
{
	void *buf = PlMAllocA( size );
	ape_mm_setup_reference( "temp", 0, m, CleanupTempAllocCallback, buf );
	return buf;
}

/**
 * Attempts to immediately free the given resource.
 * If this isn't called, resource will be cleaned
 * up automatically later.
 */
void apeTempFree( ApeMemoryReference *m )
{
	if ( !FreeReference( m, false ) )
	{
		PRINT_WARNING( "Failed to cleanup temporary pool!\n" );
	}
}
