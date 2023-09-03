// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "entity.h"

static PLHashTable *entityClassTable = NULL;

void apeRegisterEntityClass( ApeEntityClassRegisterFunction callback ) {
	if ( entityClassTable == NULL ) {
		entityClassTable = PlCreateHashTable();
	}

	const ApeEntityClassTable *classTable = callback();
	if ( PlLookupHashTableUserData( entityClassTable, classTable->name, strlen( classTable->name ) ) != NULL ) {
		PRINT_WARNING( "Attempted to register a duplicate entity class (%s)\n", classTable->name );
		return;
	}

	PlInsertHashTableNode( entityClassTable, classTable->name, strlen( classTable->name ), ( void * ) classTable );

	// call the cache function, so we can load resources into memory
	if ( classTable->Cache != NULL ) {
		classTable->Cache();
	}
}

const ApeEntityClassTable *apeGetEntityClassTable( const char *className ) {
	return ( const ApeEntityClassTable * ) PlLookupHashTableUserData( entityClassTable, className, strlen( className ) );
}

ApeEntity *apeCreateEntity( const char *className, NdBranch *properties ) {
	const ApeEntityClassTable *classTable = apeGetEntityClassTable( className );
	if ( className == NULL ) {
		PRINT_WARNING( "Failed to find entity class (%s)!\n", className );
		return NULL;
	}

	ApeEntity *entity = PL_NEW( ApeEntity );
	entity->classTable = classTable;
	entity->componentTable = PlCreateHashTable();
	if ( classTable->Create != NULL ) {
		classTable->Create( entity, properties );
	}

	return entity;
}

void apeDestroyEntity( ApeEntity *entity ) {
	PLHashTableNode *node = PlGetFirstHashTableNode( entity->componentTable );
	while ( node != NULL ) {
		//TODO: should be calling destructor for component!!!
		PL_DELETE( PlGetHashTableNodeUserData( node ) );
		node = PlGetNextHashTableNode( entity->componentTable, node );
	}
	PlDestroyHashTable( entity->componentTable );

	PL_DELETE( entity );
}

void apeTickEntity( ApeEntity *entity ) {
	assert( entity->classTable != NULL );
	if ( entity->classTable->Tick == NULL ) {
		return;
	}

	entity->classTable->Tick( entity );
}

void apeDrawEntity( ApeEntity *entity ) {
	assert( entity->classTable != NULL );
	if ( entity->classTable->Draw == NULL ) {
		return;
	}

	entity->classTable->Draw( entity );
}

const char *apeGetEntityClassName( ApeEntity *entity ) {
	return entity->classTable->name;
}

void *apeGetEntityClassData( ApeEntity *entity ) {
	return entity->classData;
}
