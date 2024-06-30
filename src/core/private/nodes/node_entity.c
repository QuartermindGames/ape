// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "node_entity.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private


static PLHashTable *entityComponentDefinitions = NULL;
static PLHashTable *entityClassDefinitions = NULL;

static void list_entity_classes_command( unsigned int, char ** )
{
	if ( entityClassDefinitions == NULL )
	{
		return;
	}

	PRINT( "Listing %u entity classes...\n", PlGetNumHashTableNodes( entityClassDefinitions ) );

	PLHashTableNode *node = PlGetFirstHashTableNode( entityClassDefinitions );
	while ( node != NULL )
	{
		ApeEntityClassDefinition *classDefinition = PlGetHashTableNodeUserData( node );
		PRINT( "-------------------------------------------------\n" );
		PRINT( "%s : %s\n", classDefinition->name, classDefinition->description != NULL ? classDefinition->description : "none" );
		PRINT( " num properties       = %u\n", classDefinition->numProperties );
		PRINT( " cache callback       = %p\n", classDefinition->cacheFunction );
		PRINT( " create callback      = %p\n", classDefinition->createFunction );
		PRINT( " destroy callback     = %p\n", classDefinition->destroyFunction );
		PRINT( " spawn callback       = %p\n", classDefinition->spawnFunction );
		PRINT( " tick callback        = %p\n", classDefinition->tickFunction );
		PRINT( " draw callback        = %p\n", classDefinition->drawFunction );
		PRINT( " serialize callback   = %p\n", classDefinition->serializeFunction );
		PRINT( " deserialize callback = %p\n", classDefinition->deserializeFunction );

		node = PlGetNextHashTableNode( node );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_register_entity_commands_( void )
{
	PlRegisterConsoleCommand( "list_entity_classes", "List all of the registered entity classes.", 0, list_entity_classes_command );
}

void ape_register_entity_class( const ApeEntityClassDefinition *definition )
{
	if ( entityClassDefinitions == NULL )
	{
		entityClassDefinitions = PlCreateHashTable();
	}

	if ( PlLookupHashTableUserData( entityClassDefinitions, definition->name, strlen( definition->name ) ) != NULL )
	{
		ape_warning_( "Attempted to register a duplicate entity class (%s)\n", definition->name );
		return;
	}

	if ( definition->createFunction == nullptr )
	{
		ape_warning_( "Encountered a class (%s) with no create callback!\n", definition->name );
		return;
	}

	PlInsertHashTableNode( entityClassDefinitions, definition->name, strlen( definition->name ), ( void * ) definition );

	// call the cache function, so we can load resources into memory
	if ( definition->cacheFunction != NULL )
	{
		definition->cacheFunction();
	}
}

const ApeEntityClassDefinition *ape_get_entity_class_table( const char *className )
{
	return ( const ApeEntityClassDefinition * ) PlLookupHashTableUserData( entityClassDefinitions, className, strlen( className ) );
}

ApeEntity *ape_create_entity( const char *className, NdBranch *properties, ApeWorldNode *parent )
{
	const ApeEntityClassDefinition *classDefinition = ape_get_entity_class_table( className );
	if ( className == NULL )
	{
		ape_warning_( "Failed to find entity class (%s)!\n", className );
		return NULL;
	}

	ApeEntity *entity = PL_NEW( ApeEntity );
	ape_world_node_setup_( &entity->base, parent, APE_WORLD_NODE_TYPE_ENTITY, &pl_vecOrigin3, &pl_vecOrigin3 );
	entity->classDefinition = classDefinition;
	entity->componentTable = PlCreateHashTable();

	entity->classData = classDefinition->createFunction( entity, properties );
	if ( entity->classData == nullptr )
	{
		ape_warning_( "Creation failed for entity (%s)!\n", entity->classDefinition->name );
		ape_world_node_destroy( ( ApeWorldNode * ) entity );
		return nullptr;
	}

	return entity;
}

void ape_entity_destroy_( void *data )
{
	ApeEntity *self = ( ApeEntity * ) data;
	if ( self == nullptr )
	{
		return;
	}

	PLHashTableNode *node = PlGetFirstHashTableNode( self->componentTable );
	while ( node != NULL )
	{
		//TODO: should be calling destructor for component!!!
		PL_DELETE( PlGetHashTableNodeUserData( node ) );
		node = PlGetNextHashTableNode( node );
	}
	PlDestroyHashTable( self->componentTable );

	PL_DELETE( self );
}

void ape_entity_tick( ApeEntity *entity )
{
	assert( entity->classDefinition != NULL );
	if ( entity->classDefinition->tickFunction == NULL )
	{
		return;
	}

	entity->classDefinition->tickFunction( entity );
}

void ape_entity_draw( ApeEntity *entity )
{
	assert( entity->classDefinition != NULL );
	if ( entity->classDefinition->drawFunction == NULL )
	{
		return;
	}

	entity->classDefinition->drawFunction( entity );
}

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
