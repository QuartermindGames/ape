// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Draw batch manager.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "ape_private.h"
#include "renderer.h"

typedef struct ApeRendererDrawBatch
{
	PLGMesh     *mesh;
	ApeMaterial *material;
	bool         usedThisFrame;

	PLLinkedListNode *linkNode;
	PLHashTableNode  *hashNode;
} ApeRendererDrawBatch;

static PLLinkedList *batches;
static PLHashTable  *batchLookup;

static void destroy_batch( void *user )
{
	ApeRendererDrawBatch *batch = user;

	if ( batch->mesh != nullptr )
	{
		PlgDestroyMesh( batch->mesh );
	}

#if 0// nah, don't do this... the original caller should be responsible
	if ( batch->material != nullptr )
	{
		ape_material_release( batch->material );
	}
#endif

	PlDestroyHashTableNode( batch->hashNode );

	qm_os_memory_free( batch );
}

static ApeRendererDrawBatch *get_batch( ApeMaterial *material )
{
	intptr_t              ptr   = ( intptr_t ) material;
	ApeRendererDrawBatch *batch = PlLookupHashTableUserData( batchLookup, &ptr, sizeof( intptr_t ) );
	if ( batch != nullptr )
	{
		return batch;
	}

	// batch for this material doesn't exist, so let's set one up

	batch           = QM_OS_MEMORY_NEW( ApeRendererDrawBatch );
	batch->mesh     = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 256, 256 );
	batch->material = material;//TODO: use shared_ptr instead...
	batch->linkNode = PlInsertLinkedListNode( batches, batch );
	batch->hashNode = PlInsertHashTableNode( batchLookup, &ptr, sizeof( intptr_t ), batch );

	return batch;
}

static void cleanup_batch_queue()
{
	ApeRendererDrawBatch *batch;
	COM_ITERATE_LINKED_LIST( batch, batches, i )
	{
		if ( batch->mesh->num_triangles != 0 )
		{
			PlgClearMesh( batch->mesh );
			continue;
		}

		PlDestroyLinkedListNode( batch->linkNode );

		destroy_batch( batch );
	}
}

void ape_renderer_batch_initialize_()
{
	batches = PlCreateLinkedList();
	if ( batches == nullptr )
	{
		ape_console_error_( true, "Failed to create batches list: %s\n", PlGetError() );
	}

	batchLookup = PlCreateHashTable();
	if ( batchLookup == nullptr )
	{
		ape_console_error_( true, "Failed to create batch lookup table: %s\n", PlGetError() );
	}
}

void ape_renderer_batch_shutdown_()
{
	if ( batches != nullptr )
	{
		PlDestroyLinkedListEx( batches, destroy_batch );
		batches = nullptr;
	}

	PlDestroyHashTable( batchLookup );
}

void ape_renderer_batch_display_()
{
	ApeRendererDrawBatch *batch;
	COM_ITERATE_LINKED_LIST( batch, batches, i )
	{
		ape_material_draw( batch->material, batch->mesh, nullptr );
	}

	cleanup_batch_queue();
}

PLGMesh *ape_renderer_batch_get_mesh( ApeMaterial *material )
{
	ApeRendererDrawBatch *batch = get_batch( material );
	if ( batch == nullptr )
	{
		return nullptr;
	}

	return batch->mesh;
}
