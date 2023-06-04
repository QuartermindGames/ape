// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_array_vector.h>

#include <yin/node.h>

#include "core_private.h"
#include "entity.h"

#define ENT_INTERNAL_TAG 32 /* maximum length of an internal tag */

static unsigned int numTotalSpawns;

/****************************************
 * ENTITY MANAGER
 ****************************************/

/****************************************
 * ENTITY
 ****************************************/

NdBranch *YnCore_Entity_Serialize( ApeEntity *self, NdBranch *root )
{
	NdBranch *entityNode     = ndPushBackObject( root, "entity" );
	NdBranch *componentsNode = ndPushBackObjectArray( entityNode, "components" );

	PLLinkedListNode *node = PlGetFirstNode( self->components );
	while ( node != NULL )
	{
		NdBranch *componentNode = ndPushBackObject( componentsNode, NULL );

		ApeEntityComponent *entityComponent = ( ApeEntityComponent * ) PlGetLinkedListNodeUserData( node );
		ndPushBackString( componentNode, "id", entityComponent->base->name );

		const ApeEntityComponentBase *entityComponentTemplate = entityComponent->base;
		if ( entityComponentTemplate->callbackTable->serializeFunction != NULL )
			entityComponentTemplate->callbackTable->serializeFunction( entityComponent, componentNode );

		node = PlGetNextLinkedListNode( node );
	}

	return entityNode;
}

ApeEntityComponent *YnCore_Entity_GetComponentByName( ApeEntity *self, const char *name )
{
	PLLinkedListNode *node = PlGetFirstNode( self->components );
	while ( node != NULL )
	{
		ApeEntityComponent *entityComponent = PlGetLinkedListNodeUserData( node );
		if ( strcmp( entityComponent->base->name, name ) == 0 )
			return entityComponent;

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

ApeEntityComponent *YnCore_Entity_AttachComponentByName( ApeEntity *self, const char *name )
{
	if ( YnCore_Entity_GetComponentByName( self, name ) != NULL )
	{
		PRINT_WARNING( "Entity already has a component of type \"%s\"!\n", name );
		return NULL;
	}

	return YnCore_EntityManager_AddComponentToEntity( self, name );
}

void apeRemoveEntityComponent( ApeEntity *self, ApeEntityComponent *component )
{
	PlDestroyLinkedListNode( component->listNode );

	const ApeEntityComponentBase *base = component->base;
	if ( base->callbackTable->destroyFunction != NULL )
		base->callbackTable->destroyFunction( component );

	PL_DELETE( component );

	PRINT_DEBUG( "Removed component (%s) from entity (%u)\n", base->name, self->id );
}

void apeRemoveAllEntityComponents( ApeEntity *self )
{
	PLLinkedListNode *node = PlGetFirstNode( self->components );
	while ( node != NULL )
	{
		ApeEntityComponent *component = PlGetLinkedListNodeUserData( node );
		node                       = PlGetNextLinkedListNode( node );
		apeRemoveEntityComponent( self, component );
	}
}
