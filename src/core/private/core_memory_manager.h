/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

void ogeInitializeMemoryManager( void );
void YnCore_ShutdownMemoryManager( void );

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
typedef struct MMCacheHeader
{
	uint32_t                 id;                /* identifier (hashed string) */
	char                     description[ 256 ];//
	uint8_t                  pool;              /* pool we're cached into */
	void                    *userData;          /* pointer to user allocated data */
	struct PLLinkedListNode *node;              /* index in pool */
} MMCacheHeader;

void  MM_AddToCache( const char *id, uint8_t pool, void *data );
void *MM_GetCachedData( const char *id, uint8_t pool );

/* ======================================================================
 * Reference Counting and Garbage Collection
 * ====================================================================*/

typedef void ( *MMReference_CleanupFunction )( void *userData );
typedef struct MMReference
{
	bool                        isInitialized;  // Indicates whether the handle was set up
	int                         numReferences;  // Number of total references
	unsigned int                timeToLive;     // Time to live
	void                       *userData;       // Pointer to original data struct
	MMCacheHeader              *cache;          // Pointer to sample on cache
	MMReference_CleanupFunction cleanupFunction;// Function that deals with the *real* cleanup
	struct PLLinkedListNode    *node;           // Index into the memory reference list
} YNCoreMemoryReference;

YNCoreMemoryReference *MemoryManager_SetupReference( const char *id, uint8_t pool, YNCoreMemoryReference *m, MMReference_CleanupFunction cleanupFunction, void *userData );

void         MemoryManager_AddReference( YNCoreMemoryReference *m );
void         MemoryManager_ReleaseReference( YNCoreMemoryReference *m );
int          MemoryManager_GetNumberOfReferences( const YNCoreMemoryReference *m );
unsigned int MemoryManager_FlushUnreferencedResources( void );

void *MM_TempAlloc( YNCoreMemoryReference *m, size_t size );
void  MM_TempFree( YNCoreMemoryReference *m );
