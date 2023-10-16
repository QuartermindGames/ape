// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "entity.h"

static PLHashTable *entityClassDefinitions = NULL;

void acl_entity_register_class( const AclEntityClassDefinition *definition ) {
	if ( entityClassDefinitions == NULL ) {
		entityClassDefinitions = PlCreateHashTable();
	}

	if ( PlLookupHashTableUserData( entityClassDefinitions, definition->name, strlen( definition->name ) ) != NULL ) {
		PRINT_WARNING( "Attempted to register a duplicate entity class (%s)\n", definition->name );
		return;
	}

	PlInsertHashTableNode( entityClassDefinitions, definition->name, strlen( definition->name ), ( void * ) definition );

	// call the cache function, so we can load resources into memory
	if ( definition->Cache != NULL ) {
		definition->Cache();
	}
}

const AclEntityClassDefinition *apeGetEntityClassTable( const char *className ) {
	return ( const AclEntityClassDefinition * ) PlLookupHashTableUserData( entityClassDefinitions, className, strlen( className ) );
}

ApeEntity *acl_entity_create( const char *className, NdBranch *properties ) {
	const AclEntityClassDefinition *classDefinition = apeGetEntityClassTable( className );
	if ( className == NULL ) {
		PRINT_WARNING( "Failed to find entity class (%s)!\n", className );
		return NULL;
	}

	ApeEntity *entity = PL_NEW( ApeEntity );
	entity->classDefinition = classDefinition;
	entity->componentTable = PlCreateHashTable();
	if ( classDefinition->Create != NULL ) {
		entity->classData = classDefinition->Create( entity, properties );
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
	assert( entity->classDefinition != NULL );
	if ( entity->classDefinition->Tick == NULL ) {
		return;
	}

	entity->classDefinition->Tick( entity );
}

void apeDrawEntity( ApeEntity *entity ) {
	assert( entity->classDefinition != NULL );
	if ( entity->classDefinition->Draw == NULL ) {
		return;
	}

	entity->classDefinition->Draw( entity );
}
