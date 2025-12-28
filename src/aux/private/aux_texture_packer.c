// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Texture/image packer. Used for lightmaps and other crap.
// Author:  Mark E. Sowden

#include "aux_private.h"

#include "../public/aux_math.h"

/////////////////////////////////////////////////////////////////////////////////////
// Texture Packer
// https://blackpawn.com/texts/lightmaps/
/////////////////////////////////////////////////////////////////////////////////////

typedef struct AuxTexturePackerNode
{
	struct AuxTexturePackerNode *child[ 2 ];
	ComMathRectI32               rect;
	void                        *data;
} AuxTexturePackerNode;

static void aux_texture_packer_node_destroy( void *ptr )
{
	if ( ptr == nullptr )
	{
		return;
	}

	AuxTexturePackerNode *node = ptr;

	// children don't have destructor callback set, so this is fine
	if ( node->child[ 0 ] != nullptr )
	{
		aux_texture_packer_node_destroy( node->child[ 0 ] );
		qm_os_memory_free( node->child[ 0 ] );
	}
	if ( node->child[ 1 ] != nullptr )
	{
		aux_texture_packer_node_destroy( node->child[ 1 ] );
		qm_os_memory_free( node->child[ 1 ] );
	}
}

AuxTexturePackerNode *aux_texture_packer_node_create_root( const unsigned int w, const unsigned int h )
{
	AuxTexturePackerNode *root = qm_os_memory_alloc( 1, sizeof( AuxTexturePackerNode ), aux_texture_packer_node_destroy );

	root->rect.w = w;
	root->rect.h = h;

	return root;
}

ComMathRectI32 aux_texture_packer_node_get_rect( const AuxTexturePackerNode *self )
{
	return self->rect;
}

AuxTexturePackerNode *aux_texture_packer_node_insert( AuxTexturePackerNode *self, const unsigned int w, const unsigned int h )
{
	if ( self->child[ 0 ] != nullptr || self->child[ 1 ] != nullptr )
	{
		// try inserting into first child
		AuxTexturePackerNode *child = aux_texture_packer_node_insert( self->child[ 0 ], w, h );
		if ( child != nullptr )
		{
			return child;
		}

		// no room, insert into second
		return aux_texture_packer_node_insert( self->child[ 1 ], w, h );
	}

	// if there's already a lightmap here, return
	if ( self->data != nullptr )
	{
		return nullptr;
	}

	// if we're too small, return
	if ( w > self->rect.w || h > self->rect.h )
	{
		return nullptr;
	}

	// if we're just right, accept
	if ( w == self->rect.w && h == self->rect.h )
	{
		self->data = ( int * ) 1;
		return self;
	}

	// otherwise, gotta split this node and create some kids
	self->child[ 0 ] = QM_OS_MEMORY_NEW( AuxTexturePackerNode );
	self->child[ 1 ] = QM_OS_MEMORY_NEW( AuxTexturePackerNode );

	// decide which way to split
	int dw = self->rect.w - w;
	int dh = self->rect.h - h;
	if ( dw > dh )
	{
		self->child[ 0 ]->rect.x = self->rect.x;
		self->child[ 0 ]->rect.y = self->rect.y;
		self->child[ 0 ]->rect.w = w;
		self->child[ 0 ]->rect.h = self->rect.h;

		self->child[ 1 ]->rect.x = self->rect.x + w;
		self->child[ 1 ]->rect.y = self->rect.y;
		self->child[ 1 ]->rect.w = self->rect.w - w;
		self->child[ 1 ]->rect.h = self->rect.h;
	}
	else
	{
		self->child[ 0 ]->rect.x = self->rect.x;
		self->child[ 0 ]->rect.y = self->rect.y;
		self->child[ 0 ]->rect.w = self->rect.w;
		self->child[ 0 ]->rect.h = h;

		self->child[ 1 ]->rect.x = self->rect.x;
		self->child[ 1 ]->rect.y = self->rect.y + h;
		self->child[ 1 ]->rect.w = self->rect.w;
		self->child[ 1 ]->rect.h = self->rect.h - h;
	}

	// insert into first child we created
	return aux_texture_packer_node_insert( self->child[ 0 ], w, h );
}
