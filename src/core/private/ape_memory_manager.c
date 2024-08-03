// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Memory management system
// Author:  Mark E. Sowden

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"

#if !defined( NDEBUG )
#define DEBUG_MEMORY
#endif

static PLMemoryGroup *memoryGroups[ APE_MAX_CACHE_POOLS ];

/* ======================================================================
 * Cache Pools
 * ====================================================================*/

static PLLinkedList *memCachePools[ APE_MAX_CACHE_POOLS ];

static void initialize_cache_pools( void )
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

void ape_cache_add_to_pool_( const char *id, ApeCachePool pool, void *data )
{
	/* ensure the data hasn't been cached already */
	void *cachedData = ape_cache_get_data_( id, pool );
	if ( cachedData != NULL )
	{
		PRINT_ERROR( "Attempted to cache duplicate data: %s\n", id );
	}

	ApeMemoryCacheHeader *header = PL_NEW( ApeMemoryCacheHeader );
	header->id                   = PlGenerateHashSDBM( id );
	header->pool                 = pool;
	header->userData             = data;
	snprintf( header->description, sizeof( header->description ), "%s", id );

	PLLinkedListNode *node = PlInsertLinkedListNode( memCachePools[ pool ], header );
	if ( node == NULL )
	{
		PRINT_ERROR( "Failed to insert node for cache pool!\n" );
	}

	PRINT( "Added \"%s\" (%u) to cache pool %u\n", id, header->id, pool );
}

static ApeMemoryCacheHeader *get_cache( uint32_t id, uint8_t pool )
{
	PLLinkedListNode *node = PlGetFirstNode( memCachePools[ pool ] );
	while ( node != NULL )
	{
		ApeMemoryCacheHeader *header = PlGetLinkedListNodeUserData( node );
		if ( header->id == id )
		{
			return header;
		}

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

void *ape_cache_get_data_( const char *id, ApeCachePool pool )
{
	uint32_t              hashedName = PlGenerateHashSDBM( id );
	ApeMemoryCacheHeader *header     = get_cache( hashedName, pool );
	if ( header != NULL )
	{
		return header->userData;
	}

	return NULL;
}

static void remove_from_cache( uint32_t id, uint8_t pool )
{
	ApeMemoryCacheHeader *header = get_cache( id, pool );
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

static bool free_reference( ApeMemoryReference *m, bool force )
{
#if 0
#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "%s, numRefs = %d, ttl = %u\n",
	             m->cache == NULL ? "unknown" : m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#endif
#endif

	if ( m->numReferences <= 0 && ( force || m->timeToLive < ape_get_num_ticks() ) )
	{
#if defined( DEBUG_MEMORY )
		PRINT_DEBUG( "Freeing reference: %s\n", m->id );
#endif

		/* remove it from whatever cached list it exists in */
		if ( m->cache != NULL )
		{
			remove_from_cache( m->cache->id, m->cache->pool );
			m->cache = NULL;
		}

		PLLinkedListNode *node = m->node;
		m->cleanupFunction( m->userData );
		PlDestroyLinkedListNode( node );

		return true;
	}

	return false;
}

static void cleanup_unreferenced_resources( bool force )
{
	PLLinkedListNode *child = PlGetFirstNode( mmReferenceList );
	while ( child != NULL )
	{
		PLLinkedListNode   *nextChild = PlGetNextLinkedListNode( child );
		ApeMemoryReference *m         = PlGetLinkedListNodeUserData( child );

#if 0
#if defined( DEBUG_MEMORY )
		PRINT_DEBUG( " %s (" PL_FMT_int32 ")\n",
		             m->cache == NULL ? "unknown" : m->cache->description,
		             m->numReferences );
#endif
#endif

		free_reference( m, force );

		child = nextChild;
	}
}

#define MEM_CLEANUP_TASK_NAME "mem_cleanup"

static void cleanup_callback( PL_UNUSED void *unused0, PL_UNUSED double unused1 )
{
	cleanup_unreferenced_resources( false );

	apePushScheduledTask( MEM_CLEANUP_TASK_NAME, cleanup_callback, NULL, MEM_CLEANUP_DELAY );
}

void ape_initialize_memory_manager_( void )
{
	PRINT( "Initializing memory manager\n" );

	for ( unsigned int i = 0; i < APE_MAX_CACHE_POOLS; ++i )
		memoryGroups[ i ] = PlCreateMemoryGroup();

	initialize_cache_pools();

	mmReferenceList = PlCreateLinkedList();
	if ( mmReferenceList == NULL )
	{
		PRINT_ERROR( "Failed to create memory manager linked list!\n" );
	}

	apePushScheduledTask( MEM_CLEANUP_TASK_NAME, cleanup_callback, NULL, MEM_CLEANUP_DELAY );
}

void ape_shutdown_memory_manager_( void )
{
	ape_memory_manager_flush_unreferenced_resources();

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

unsigned int ape_memory_manager_flush_unreferenced_resources( void )
{
	unsigned int references = PlGetNumLinkedListNodes( mmReferenceList );
	while ( references > 0 )
	{
#if defined( DEBUG_MEMORY )
		PRINT_DEBUG( " dangling references: " PL_FMT_uint32 "\n", references );
#endif

		cleanup_unreferenced_resources( true );

		unsigned int n = PlGetNumLinkedListNodes( mmReferenceList );
		if ( n == references )
		{
			break;
		}

		references = n;
	}

	return references;
}

ApeMemoryReference *ape_mm_setup_reference( const char *id, uint8_t pool, ApeMemoryReference *m, MMReference_CleanupFunction cleanupFunction, void *userData )
{
	snprintf( m->id, sizeof( m->id ), "%s", id );
	m->cache           = ape_cache_get_data_( id, pool );
	m->userData        = userData;
	m->cleanupFunction = cleanupFunction;
	m->isInitialized   = true;
	m->node            = PlInsertLinkedListNode( mmReferenceList, m );
	return m;
}

void ape_mm_add_reference( ApeMemoryReference *m )
{
	m->numReferences++;
	m->timeToLive = ( ape_get_num_ticks() + 1024 );
#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "Adding reference: %s (%s) (%d) (%u)\n",
	             m->id,
	             m->cache == NULL ? "unknown" : m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#endif
}

void ape_mm_release( ApeMemoryReference *m )
{
	assert( m->numReferences > 0 );

#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "Releasing reference: %s (%s) (%d) (%u)\n",
	             m->id,
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

static void cleanup_temp_alloc_callback( void *userData )
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
	ape_mm_setup_reference( "temp", 0, m, cleanup_temp_alloc_callback, buf );
	return buf;
}

/**
 * Attempts to immediately free the given resource.
 * If this isn't called, resource will be cleaned
 * up automatically later.
 */
void apeTempFree( ApeMemoryReference *m )
{
	if ( !free_reference( m, false ) )
	{
		PRINT_WARNING( "Failed to cleanup temporary pool!\n" );
	}
}
