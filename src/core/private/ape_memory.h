// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

void ape_memory_initialize_( void );
void ape_memory_shutdown_( void );

/* ======================================================================
 * Active Cache
 * ====================================================================*/

typedef enum ApeMemoryCachePool
{
	APE_CACHE_POOL_FONTS,
	APE_CACHE_POOL_TEXTURES,
	APE_CACHE_POOL_MATERIALS,
	APE_CACHE_POOL_MODELS,
	APE_CACHE_POOL_PARTICLES,
	APE_CACHE_POOL_SAMPLES,

	APE_CACHE_POOL_WORLDS,
	APE_CACHE_POOL_WORLD_MESHES,

	APE_MAX_CACHE_POOLS
} ApeMemoryCachePool;

/**
 * Header for cached data item.
 */
typedef struct ApeMemoryCacheHeader
{
	uint32_t                 id;                /* identifier (hashed string) */
	char                     description[ 256 ];//
	uint8_t                  pool;              /* pool we're cached into */
	void                    *userData;          /* pointer to user allocated data */
	struct PLLinkedListNode *node;              /* index in pool */
} ApeMemoryCacheHeader;

void *ape_memory_get_from_pool_( const char *id, ApeMemoryCachePool pool );

PLLinkedList *ape_memory_get_pool_list_( ApeMemoryCachePool pool );

/* ======================================================================
 * Reference Counting and Garbage Collection
 * ====================================================================*/

typedef void ( *ApeMemoryCleanupCallback )( void *userData );
typedef struct ApeMemoryReference
{
	bool                     isInitialized;  // Indicates whether the handle was set up
	int                      numReferences;  // Number of total references
	unsigned int             timeToLive;     // Time to live
	void                    *userData;       // Pointer to original data struct
	ApeMemoryCacheHeader    *cache;          // Pointer to sample on cache
	ApeMemoryCleanupCallback cleanupFunction;// Function that deals with the *real* cleanup
	struct PLLinkedListNode *node;           // Index into the memory reference list
} ApeMemoryReference;

ApeMemoryReference *ape_memory_setup_reference( const char *id, uint8_t pool, ApeMemoryReference *m, ApeMemoryCleanupCallback cleanupFunction, void *userData );

void         ape_memory_add_reference( ApeMemoryReference *m );
void         ape_memory_release( ApeMemoryReference *m );
int          ape_memory_get_num_references( const ApeMemoryReference *m );
unsigned int ape_memory_flush_unreferenced_resources( void );

void *ape_memory_temp_alloc( ApeMemoryReference *m, size_t size );
void  ape_memory_temp_free( ApeMemoryReference *m );
