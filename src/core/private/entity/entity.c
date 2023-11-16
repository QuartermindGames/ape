// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "entity.h"

static PLHashTable *entityClassDefinitions = NULL;

void ss_acl_register_entity_class( const SS_Acl_EntityClassDefinition *definition ) {
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

const SS_Acl_EntityClassDefinition *ss_acl_get_entity_class_table( const char *className ) {
	return ( const SS_Acl_EntityClassDefinition * ) PlLookupHashTableUserData( entityClassDefinitions, className, strlen( className ) );
}

SS_Acl_Entity *ss_acl_entity_create( const char *className, NdBranch *properties ) {
	const SS_Acl_EntityClassDefinition *classDefinition = ss_acl_get_entity_class_table( className );
	if ( className == NULL ) {
		PRINT_WARNING( "Failed to find entity class (%s)!\n", className );
		return NULL;
	}

	SS_Acl_Entity *entity = PL_NEW( SS_Acl_Entity );
	entity->classDefinition = classDefinition;
	entity->componentTable = PlCreateHashTable();
	if ( classDefinition->Create != NULL ) {
		entity->classData = classDefinition->Create( entity, properties );
	}

	return entity;
}

void ss_acl_entity_destroy( SS_Acl_Entity *entity ) {
	PLHashTableNode *node = PlGetFirstHashTableNode( entity->componentTable );
	while ( node != NULL ) {
		//TODO: should be calling destructor for component!!!
		PL_DELETE( PlGetHashTableNodeUserData( node ) );
		node = PlGetNextHashTableNode( entity->componentTable, node );
	}
	PlDestroyHashTable( entity->componentTable );

	PL_DELETE( entity );
}

void ss_acl_entity_tick( SS_Acl_Entity *entity ) {
	assert( entity->classDefinition != NULL );
	if ( entity->classDefinition->Tick == NULL ) {
		return;
	}

	entity->classDefinition->Tick( entity );
}

void ss_acl_entity_draw( SS_Acl_Entity *entity ) {
	assert( entity->classDefinition != NULL );
	if ( entity->classDefinition->Draw == NULL ) {
		return;
	}

	entity->classDefinition->Draw( entity );
}
