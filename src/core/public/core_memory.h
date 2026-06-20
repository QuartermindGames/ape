// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/////////////////////////////////////////////////////////////////////////////////////
// Reference Counting and Garbage Collection
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeMemoryReference ApeMemoryReference;

void ape_memory_reference_add( ApeMemoryReference *m );
void ape_memory_reference_release( ApeMemoryReference *m );

// these are here to ensure a consistent interface for referenced objects -
// if you're implementing something using this API, use them!
#define APE_MEMORY_IMPLEMENT_INTERFACE( PREFIX, TYPE, VAR )                             \
	void PREFIX##_add_reference( TYPE *ptr ) { ape_memory_reference_add( &ptr->VAR ); } \
	void PREFIX##_release_reference( TYPE *ptr ) { ape_memory_reference_release( &ptr->VAR ); }
#define APE_MEMORY_IMPLEMENT_INTERFACE_DECL( PREFIX, TYPE ) \
	void PREFIX##_add_reference( TYPE *ptr );               \
	void PREFIX##_release_reference( TYPE *ptr );

/////////////////////////////////////////////////////////////////////////////////////
// Alloc
// These are basically just wrappers on top of QMFWs memory methods.
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Wrapper around qm_os_memory_alloc.
 * If die is true, it'll bail on failure.
 */
void *ape_memory_calloc( size_t num, size_t size, void ( *destructor )( void * ), bool die );

/**
 * Wrapper around qm_os_memory_alloc.
 * If die is true, it'll bail on failure.
 */
void *ape_memory_alloc( size_t size, void ( *destructor )( void * ), bool die );

// these just replicate the qmfw macros, for ease
// they'll always die on failure though so if you
// don't want that, use the above methods instead
#define APE_MEMORY_NEW( TYPE )                     ( TYPE * ) ape_memory_alloc( sizeof( TYPE ), nullptr, true )
#define APE_MEMORY_NEW_C( TYPE, NUM )              ( TYPE * ) ape_memory_calloc( NUM, sizeof( TYPE ), nullptr, true )
#define APE_MEMORY_NEW_D( TYPE, DESTRUCTOR )       ( TYPE * ) ape_memory_alloc( sizeof( TYPE ), DESTRUCTOR, true )
#define APE_MEMORY_NEW_CD( TYPE, NUM, DESTRUCTOR ) ( TYPE * ) ape_memory_calloc( NUM, sizeof( TYPE ), DESTRUCTOR, true )
