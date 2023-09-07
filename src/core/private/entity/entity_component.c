// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
/* This essentially consolidates the entity component system into one file, whereas
 * before it was all spread through entity.c, which made it really confusing... */

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "entity.h"

static PLHashTable *entityComponentTable = NULL;

void apeRegisterEntityComponent( ApeEntityComponentRegisterFunction callback ) {
	if ( entityComponentTable == NULL ) {
		entityComponentTable = PlCreateHashTable();
	}

	const ApeEntityComponentTable *componentTable = callback();
	if ( PlLookupHashTableUserData( entityComponentTable, componentTable->name, strlen( componentTable->name ) ) != NULL ) {
		PRINT_WARNING( "Attempted to register a duplicate entity component (%s)\n", componentTable->name );
		return;
	}

	PlInsertHashTableNode( entityComponentTable, componentTable->name, strlen( componentTable->name ), ( void * ) componentTable );
}

void *apeAddEntityComponent( ApeEntity *entity, const char *name ) {
	const ApeEntityComponentTable *componentTable = PlLookupHashTableUserData( entityComponentTable, name, strlen( name ) );
	if ( componentTable == NULL ) {
		PRINT_WARNING( "Failed to find entity component (%s)!\n", name );
		return NULL;
	}

	ApeEntityComponent *component = PL_NEW( ApeEntityComponent );
	if ( componentTable->Create != NULL ) {
		component->data = componentTable->Create();
	}

	if ( !PlInsertHashTableNode( entity->componentTable, name, strlen( name ), component ) ) {
		PRINT_WARNING( "Failed to insert entity component (%s): %s\n", name, PlGetError() );

		if ( componentTable->Destroy != NULL ) {
			componentTable->Destroy( component );
		}

		PL_DELETE( component );
		return NULL;
	}

	return component->data;
}

void *apeGetEntityComponent( ApeEntity *entity, const char *name ) {
	return PlLookupHashTableUserData( entity->componentTable, name, strlen( name ) );
}
