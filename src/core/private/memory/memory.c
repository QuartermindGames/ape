// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Memory management system
// Author:  Mark E. Sowden

#include <plcore/pl_linkedlist.h>

#include "../ape_private.h"

#if !defined( NDEBUG )
//#	define DEBUG_MEMORY
#endif

/////////////////////////////////////////////////////////////////////////////////////

static PLMemoryGroup *cacheMemoryGroups[ APE_MAX_CACHE_POOLS ];
static PLLinkedList  *cachePoolsList[ APE_MAX_CACHE_POOLS ];

/////////////////////////////////////////////////////////////////////////////////////
// Resource Cache
/////////////////////////////////////////////////////////////////////////////////////

static void initialize_cache_pools( void )
{
	for ( unsigned int i = 0; i < APE_MAX_CACHE_POOLS; ++i )
	{
		cacheMemoryGroups[ i ] = PlCreateMemoryGroup();
		if ( cacheMemoryGroups[ i ] == nullptr )
		{
			ape_error_( true, "Failed to create memory group (%u): %s\n", i, PlGetError() );
		}
		cachePoolsList[ i ] = PlCreateLinkedList();
		if ( cachePoolsList[ i ] == NULL )
		{
			ape_error_( true, "Failed to create cache pool (%u): %s\n", i, PlGetError() );
		}
	}
}

static ApeMemoryCacheHeader *add_to_cache_pool_( const char *id, ApeMemoryCachePool pool, void *data )
{
	/* ensure the data hasn't been cached already */
	void *cachedData = ape_memory_get_from_pool_( id, pool );
	if ( cachedData != NULL )
	{
		ape_error_( true, "Attempted to cache duplicate data: %s\n", id );
	}

	ApeMemoryCacheHeader *header = QM_OS_MEMORY_NEW( ApeMemoryCacheHeader );
	snprintf( header->description, sizeof( header->description ), "%s", id );
	header->id       = PlGenerateHashSDBM( id );
	header->pool     = pool;
	header->userData = data;
	header->node     = PlInsertLinkedListNode( cachePoolsList[ pool ], header );
	if ( header->node == NULL )
	{
		ape_error_( true, "Failed to insert node for cache pool!\n" );
	}

	ape_print_( "Added \"%s\" (%u) to cache pool %u\n", id, header->id, pool );

	return header;
}

static ApeMemoryCacheHeader *get_cache( uint32_t id, uint8_t pool )
{
	assert( cachePoolsList[ pool ] != NULL );

	PLLinkedListNode *node = PlGetFirstNode( cachePoolsList[ pool ] );
	while ( node != NULL )
	{
		ApeMemoryCacheHeader *header = PlGetLinkedListNodeUserData( node );
		if ( header->id == id )
		{
			return header;
		}

		node = PlGetNextLinkedListNode( node );
	}

	return nullptr;
}

void *ape_memory_get_from_pool_( const char *id, ApeMemoryCachePool pool )
{
	uint32_t              hashedName = PlGenerateHashSDBM( id );
	ApeMemoryCacheHeader *header     = get_cache( hashedName, pool );
	if ( header != NULL )
	{
		return header->userData;
	}

	return NULL;
}

PLLinkedList *ape_memory_get_pool_list_( ApeMemoryCachePool pool )
{
	return cachePoolsList[ pool ];
}

static void remove_from_cache( uint32_t id, uint8_t pool )
{
	ape_print_( "Removing %u from pool %u\n", id, pool );

	return;
	ApeMemoryCacheHeader *header = get_cache( id, pool );
	if ( header == NULL )
	{
		ape_warning_( "Attempted to remove node from cache pool, but failed: %s\n", id );
		return;
	}

	PlDestroyLinkedListNode( header->node );

	ape_print_( "Removed \"%s\" from cache\n", header->description );

	qm_os_memory_free( header );
}

/////////////////////////////////////////////////////////////////////////////////////
// Reference Counting and Garbage Collection
/////////////////////////////////////////////////////////////////////////////////////

static PLLinkedList *mmReferenceList;

#define MEM_CLEANUP_DELAY 200.0

//#define DEBUG_MEMORY

static bool free_reference( ApeMemoryReference *m, bool force )
{
	return false;

#if 0
#	if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "%s, numRefs = %d, ttl = %u\n",
	             m->cache == NULL ? "unknown" : m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#	endif
#endif

	if ( m->numReferences <= 0 && ( force || m->timeToLive < ape_get_num_ticks() ) )
	{
#if defined( DEBUG_MEMORY )
		PRINT_DEBUG( "Freeing reference: %s\n", m->cache->description );
#endif

		/* remove it from whatever cached list it exists in */
		if ( m->cache != NULL )
		{
			remove_from_cache( m->cache->id, m->cache->pool );
			m->cache = nullptr;
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
#	if defined( DEBUG_MEMORY )
		PRINT_DEBUG( " %s (" PL_FMT_int32 ")\n",
		             m->cache == NULL ? "unknown" : m->cache->description,
		             m->numReferences );
#	endif
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

void ape_memory_initialize_( void )
{
	ape_print_( "Initializing memory manager\n" );

	initialize_cache_pools();

	mmReferenceList = PlCreateLinkedList();
	if ( mmReferenceList == NULL )
	{
		ape_error_( true, "Failed to create memory manager linked list!\n" );
	}

	apePushScheduledTask( MEM_CLEANUP_TASK_NAME, cleanup_callback, NULL, MEM_CLEANUP_DELAY );
}

void ape_memory_shutdown_( void )
{
	ape_memory_flush_unreferenced_resources();

	unsigned int danglingReferences = PlGetNumLinkedListNodes( mmReferenceList );
	if ( danglingReferences > 0 )
	{
		ape_warning_( "Shutting down memory manager with %u dangling references!\n", danglingReferences );
	}

	for ( unsigned int i = 0; i < APE_MAX_CACHE_POOLS; ++i )
	{
		PlDestroyMemoryGroup( cacheMemoryGroups[ i ] );
		cacheMemoryGroups[ i ] = nullptr;

		PlDestroyLinkedList( cachePoolsList[ i ] );
		cachePoolsList[ i ] = nullptr;
	}
}

unsigned int ape_memory_flush_unreferenced_resources( void )
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

ApeMemoryReference *ape_memory_setup_reference( const char *id, uint8_t pool, ApeMemoryReference *m, ApeMemoryCleanupCallback cleanupFunction, void *userData )
{
	m->cache = ape_memory_get_from_pool_( id, pool );
	if ( m->cache == nullptr )
	{
		m->cache = add_to_cache_pool_( id, pool, userData );
	}

	m->userData        = userData;
	m->cleanupFunction = cleanupFunction;
	m->isInitialized   = true;
	m->node            = PlInsertLinkedListNode( mmReferenceList, m );
	return m;
}

void ape_memory_add_reference( ApeMemoryReference *m )
{
	m->numReferences++;
	m->timeToLive = ( ape_get_num_ticks() + 1024 );
#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "Adding reference: %s (%d) (%u)\n",
	             m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#endif
}

void ape_memory_release( ApeMemoryReference *m )
{
	//assert( m->numReferences > 0 );

#if defined( DEBUG_MEMORY )
	PRINT_DEBUG( "Releasing reference: %s (%d) (%u)\n",
	             m->cache->description,
	             m->numReferences,
	             m->timeToLive );
#endif

	m->numReferences--;
	if ( m->numReferences <= 0 )
	{
		m->timeToLive = ape_get_num_ticks() + 1024;
	}
}

int ape_memory_get_num_references( const ApeMemoryReference *m )
{
	return m->numReferences;
}

/////////////////////////////////////////////////////////////////////////////////////
// Temporary Memory
/////////////////////////////////////////////////////////////////////////////////////

static void cleanup_temp_alloc_callback( void *userData )
{
	qm_os_memory_free( userData );
}

/**
 * Allocates a pool of memory that will be automatically
 * cleaned up.
 */
void *ape_memory_temp_alloc( ApeMemoryReference *m, size_t size )
{
	void *buf = QM_OS_MEMORY_MALLOC_( size );
	ape_memory_setup_reference( "temp", 0, m, cleanup_temp_alloc_callback, buf );
	return buf;
}

/**
 * Attempts to immediately free the given resource.
 * If this isn't called, resource will be cleaned
 * up automatically later.
 */
void ape_memory_temp_free( ApeMemoryReference *m )
{
	if ( !free_reference( m, false ) )
	{
		ape_warning_( "Failed to cleanup temporary pool!\n" );
	}
}
