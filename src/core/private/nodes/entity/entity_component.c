// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
/* This essentially consolidates the entity component system into one file, whereas
 * before it was all spread through entity.c, which made it really confusing... */

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "entity.h"

static PLHashTable *entityComponentDefinitions = NULL;

void ape_register_entity_component( const ApeEntityComponentDefinition *definition )
{
	if ( entityComponentDefinitions == NULL )
		entityComponentDefinitions = PlCreateHashTable();

	if ( PlLookupHashTableUserData( entityComponentDefinitions, definition->name, strlen( definition->name ) ) != NULL )
	{
		PRINT_WARNING( "Attempted to register a duplicate entity component (%s)\n", definition->name );
		return;
	}

	PlInsertHashTableNode( entityComponentDefinitions, definition->name, strlen( definition->name ), ( void * ) definition );
}

void *ss_acl_entity_add_component( ApeEntity *entity, const char *name )
{
	const ApeEntityComponentDefinition *componentDefinition = PlLookupHashTableUserData( entityComponentDefinitions, name, strlen( name ) );
	if ( componentDefinition == NULL )
	{
		PRINT_WARNING( "Failed to find entity component (%s)!\n", name );
		return NULL;
	}

	ApeEntityComponent *component = PL_NEW( ApeEntityComponent );
	if ( componentDefinition->Create != NULL )
		component->data = componentDefinition->Create();

	if ( !PlInsertHashTableNode( entity->componentTable, name, strlen( name ), component ) )
	{
		PRINT_WARNING( "Failed to insert entity component (%s): %s\n", name, PlGetError() );

		if ( componentDefinition->Destroy != NULL )
			componentDefinition->Destroy( component );

		PL_DELETE( component );
		return NULL;
	}

	return component->data;
}

void *ss_acl_entity_get_component( ApeEntity *entity, const char *name )
{
	return PlLookupHashTableUserData( entity->componentTable, name, strlen( name ) );
}
