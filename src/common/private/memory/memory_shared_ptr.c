// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Memory management system (pulled from Core)
// Author:  Mark E. Sowden

#include "common_private.h"

typedef struct ComSharedPtr
{
	int         numRefs;
	void       *ptr;
} ComSharedPtr;

ComSharedPtr *com_shared_ptr_create( void *ptr )
{
	ComSharedPtr *ref = PL_NEW( ComSharedPtr );
	ref->ptr          = ptr;
	ref->numRefs      = 1;
	return ref;
}

void com_shared_ptr_add( ComSharedPtr *self )
{
	self->numRefs++;
}

void com_shared_ptr_release( ComSharedPtr *self )
{
	if ( self->numRefs == 0 )
	{
		com_warning_( "Attempted to release shared pointer with zero references!\n" );
		return;
	}

	self->numRefs--;
	if ( self->numRefs == 0 )
	{
		if ( self->ptr != NULL )
		{
			com_warning_( "Freeing shared pointer reference, but source object hasn't been destroyed!\n" );
			//TODO: call cleanup method here?
		}
		PL_DELETE( self );
	}
}

void *com_shared_ptr_get( const ComSharedPtr *self )
{
	return self->ptr;
}

void com_shared_ptr_set( ComSharedPtr *self, void *ptr )
{
	self->ptr = ptr;
}
