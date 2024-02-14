/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

void ape_initialize_memory_manager_( void );
void ape_shutdown_memory_manager_( void );

/* ======================================================================
 * Active Cache
 * ====================================================================*/

typedef enum ApeCachePool
{
	APE_CACHE_POOL_FONTS,
	APE_CACHE_POOL_TEXTURES,
	APE_CACHE_POOL_MATERIALS,
	APE_CACHE_POOL_MODELS,
	APE_CACHE_POOL_PARTICLES,

	APE_CACHE_POOL_WORLDS,
	APE_CACHE_POOL_WORLD_MESHES,

	APE_MAX_CACHE_POOLS
} ApeCachePool;

/**
 * Header for cached data item.
 */
typedef struct ApeMemoryCacheHeader
{
	uint32_t id;                   /* identifier (hashed string) */
	char description[ 256 ];       //
	uint8_t pool;                  /* pool we're cached into */
	void *userData;                /* pointer to user allocated data */
	struct PLLinkedListNode *node; /* index in pool */
} ApeMemoryCacheHeader;

void apeAddToCachePool( const char *id, ApeCachePool pool, void *data );
void *apeGetCachedData( const char *id, ApeCachePool pool );

/* ======================================================================
 * Reference Counting and Garbage Collection
 * ====================================================================*/

typedef void ( *MMReference_CleanupFunction )( void *userData );
typedef struct ApeMemoryReference
{
	bool isInitialized;                         // Indicates whether the handle was set up
	int numReferences;                          // Number of total references
	unsigned int timeToLive;                    // Time to live
	void *userData;                             // Pointer to original data struct
	ApeMemoryCacheHeader *cache;                // Pointer to sample on cache
	MMReference_CleanupFunction cleanupFunction;// Function that deals with the *real* cleanup
	struct PLLinkedListNode *node;              // Index into the memory reference list
} ApeMemoryReference;

ApeMemoryReference *ape_mm_setup_reference( const char *id, uint8_t pool, ApeMemoryReference *m, MMReference_CleanupFunction cleanupFunction, void *userData );

void ape_mm_add_reference( ApeMemoryReference *m );
void ss_acl_mm_release( ApeMemoryReference *m );
int apeGetNumberOfReferences( const ApeMemoryReference *m );
unsigned int apeFlushUnreferencedResources( void );

void *apeTempAlloc( ApeMemoryReference *m, size_t size );
void apeTempFree( ApeMemoryReference *m );
