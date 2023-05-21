/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

void ogeInitializeMemoryManager( void );
void ogeShutdownMemoryManager( void );

/* ======================================================================
 * Active Cache
 * ====================================================================*/

enum MMCachePool
{
	MEM_CACHE_FONT,
	MEM_CACHE_TEXTURES,
	MEM_CACHE_MATERIALS,
	MEM_CACHE_MODELS,
	MEM_CACHE_PARTICLES,

	MEM_CACHE_WORLD,
	MEM_CACHE_WORLD_MESH,

	MEM_CACHE_END
};

/**
 * Header for cached data item.
 */
typedef struct ogeMemoryCacheHeader
{
	uint32_t id;                   /* identifier (hashed string) */
	char description[ 256 ];       //
	uint8_t pool;                  /* pool we're cached into */
	void *userData;                /* pointer to user allocated data */
	struct PLLinkedListNode *node; /* index in pool */
} OgeMemoryCacheHeader;

void ogeMM_AddToCache( const char *id, uint8_t pool, void *data );
void *ogeMM_GetCachedData( const char *id, uint8_t pool );

/* ======================================================================
 * Reference Counting and Garbage Collection
 * ====================================================================*/

typedef void ( *MMReference_CleanupFunction )( void *userData );
typedef struct OgeMemoryReference
{
	bool isInitialized;                         // Indicates whether the handle was set up
	int numReferences;                          // Number of total references
	unsigned int timeToLive;                    // Time to live
	void *userData;                             // Pointer to original data struct
	OgeMemoryCacheHeader *cache;                // Pointer to sample on cache
	MMReference_CleanupFunction cleanupFunction;// Function that deals with the *real* cleanup
	struct PLLinkedListNode *node;              // Index into the memory reference list
} OgeMemoryReference;

OgeMemoryReference *ogeMemoryManager_SetupReference( const char *id, uint8_t pool, OgeMemoryReference *m, MMReference_CleanupFunction cleanupFunction, void *userData );

void ogeMemoryManager_AddReference( OgeMemoryReference *m );
void ogeMemoryManager_ReleaseReference( OgeMemoryReference *m );
int ogeMemoryManager_GetNumberOfReferences( const OgeMemoryReference *m );
unsigned int ogeMemoryManager_FlushUnreferencedResources( void );

void *ogeTempAlloc( OgeMemoryReference *m, size_t size );
void ogeTempFree( OgeMemoryReference *m );
