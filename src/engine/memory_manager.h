/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

void MEM_Initialize( void );
void MEM_Shutdown( void );

#define MEM_MAX_ID PL_SYSTEM_MAX_PATH

enum MEMCachePool
{
	MEM_CACHE_FONT,
	MEM_CACHE_TEXTURES,
	MEM_CACHE_MATERIALS,
	MEM_CACHE_MODELS,

	MEM_CACHE_WORLD,
	MEM_CACHE_WORLD_MESH,

	MEM_CACHE_END
};

/**
 * Header for cached data item.
 */
typedef struct MEMCacheHeader
{
	char                     id[ MEM_MAX_ID ]; /* identifier */
	uint8_t                  pool;             /* pool we're cached into */
	void *                   userData;         /* pointer to user allocated data */
	struct PLLinkedListNode *node;             /* index in pool */
} MEMCacheHeader;

void  MEM_CacheData( const char *id, uint8_t pool, void *data );
void *MEM_GetCachedData( const char *id, uint8_t pool );

/* ======================================================================
 * Reference Counting and Garbage Collection
 * ====================================================================*/

typedef void ( *MEMReference_CleanupFunction )( void *userData );
typedef struct MEMReference
{
	bool                         isInitialized;    // Indicates whether or not the handle was setup
	char                         description[ 16 ];// Simple description, for debugging
	int                          numRefs;          // Number of total references
	unsigned int                 ttl;              // Time to live
	void *                       userData;         // Pointer to original data struct
	MEMReference_CleanupFunction cleanupFunction;  // Function that deals with the *real* cleanup
	struct PLLinkedListNode *    node;             // Index into the memory reference list
} MEMReference;

MEMReference *MEM_SetupReferenceInstance( const char *description, MEMReference *m, MEMReference_CleanupFunction cleanupFunction, void *userData );

void MEM_AddReference( MEMReference *m );
void MEM_ReleaseReference( MEMReference *m );
bool MEM_FreeReference( MEMReference *m, bool force );
int  MEM_GetNumberOfReferences( const MEMReference *m );

void *MEM_TempAlloc( MEMReference *m, size_t size );
void  MEM_TempFree( MEMReference *m );
