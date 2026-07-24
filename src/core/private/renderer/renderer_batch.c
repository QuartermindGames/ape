// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Draw batch manager.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "ape_private.h"
#include "renderer.h"

typedef struct ApeRendererDrawBatch
{
	QmGfxMesh   *mesh;
	ApeMaterial *material;
	bool         usedThisFrame;

	QmOsLinkedListNode *linkNode;
	PLHashTableNode    *hashNode;
} ApeRendererDrawBatch;

static QmOsLinkedList *batches;
static PLHashTable    *batchLookup;

static void destroy_batch( ApeRendererDrawBatch *batch )
{
	if ( batch->mesh != nullptr )
	{
		PlgDestroyMesh( batch->mesh );
	}

#if 0// nah, don't do this... the original caller should be responsible
	if ( batch->material != nullptr )
	{
		ape_material_release_reference( batch->material );
	}
#endif

	qm_os_memory_free( batch->hashNode );
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
	batch->mesh     = PlgCreateMesh( QM_GFX_MESH_PRIMITIVE_TRIANGLES, QM_GFX_MESH_DRAW_MODE_STREAM, 256, 256 );
	batch->material = material;//TODO: use shared_ptr instead...
	batch->linkNode = qm_os_linked_list_push_back( batches, batch );
	batch->hashNode = PlInsertHashTableNode( batchLookup, &ptr, sizeof( intptr_t ), batch );

	return batch;
}

static void cleanup_batch_queue()
{
	ApeRendererDrawBatch *batch;
	QM_OS_LINKED_LIST_ITERATE( batch, batches, i )
	{
		if ( batch->mesh->num_triangles != 0 )
		{
			PlgClearMesh( batch->mesh );
			continue;
		}

		qm_os_memory_free( batch->linkNode );

		destroy_batch( batch );
	}
}

void ape_renderer_batch_initialize_()
{
	batches = qm_os_linked_list_create();
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
		ApeRendererDrawBatch *batch;
		QM_OS_LINKED_LIST_ITERATE( batch, batches, i )
		{
			destroy_batch( batch );
		}

		qm_os_memory_free( batches );
		batches = nullptr;
	}

	PlDestroyHashTable( batchLookup );
}

void ape_renderer_batch_display_()
{
	ApeRendererDrawBatch *batch;
	QM_OS_LINKED_LIST_ITERATE( batch, batches, i )
	{
		ape_material_draw( batch->material, batch->mesh, nullptr );
	}

	cleanup_batch_queue();
}

QmGfxMesh *ape_renderer_batch_get_mesh( ApeMaterial *material )
{
	ApeRendererDrawBatch *batch = get_batch( material );
	if ( batch == nullptr )
	{
		return nullptr;
	}

	return batch->mesh;
}
