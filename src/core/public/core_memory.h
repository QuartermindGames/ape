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
/////////////////////////////////////////////////////////////////////////////////////
