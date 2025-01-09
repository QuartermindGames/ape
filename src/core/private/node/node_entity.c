// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "node_entity.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static PLHashTable   *entityComponentDefinitions;
static PLHashTable   *entityClassLookup;
static PLVectorArray *entityClasses;

static void list_entity_classes_command( unsigned int, char ** )
{
	if ( entityClassLookup == NULL )
	{
		return;
	}

	ape_print_( "Listing %u entity classes...\n", PlGetNumHashTableNodes( entityClassLookup ) );

	PLHashTableNode *node = PlGetFirstHashTableNode( entityClassLookup );
	while ( node != NULL )
	{
		ApeEntityClassDefinition *classDefinition = PlGetHashTableNodeUserData( node );
		ape_print_( "-------------------------------------------------\n" );
		ape_print_( "%s : %s\n", classDefinition->name, classDefinition->description != NULL ? classDefinition->description : "none" );
		ape_print_( " num properties       = %u\n", classDefinition->numProperties );
		ape_print_( " cache callback       = %p\n", classDefinition->cacheFunction );
		ape_print_( " create callback      = %p\n", classDefinition->createFunction );
		ape_print_( " destroy callback     = %p\n", classDefinition->destroyFunction );
		ape_print_( " spawn callback       = %p\n", classDefinition->spawnFunction );
		ape_print_( " tick callback        = %p\n", classDefinition->tickFunction );
		ape_print_( " draw callback        = %p\n", classDefinition->drawFunction );
		ape_print_( " serialize callback   = %p\n", classDefinition->serializeFunction );
		ape_print_( " deserialize callback = %p\n", classDefinition->deserializeFunction );

		node = PlGetNextHashTableNode( node );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_entity_register_commands_( void )
{
	PlRegisterConsoleCommand( "list_entity_classes", "List all of the registered entity classes.", 0, list_entity_classes_command );
}

void ape_register_entity_class( const ApeEntityClassDefinition *definition )
{
	if ( entityClassLookup == NULL )
	{
		entityClassLookup = PlCreateHashTable();
		entityClasses     = PlCreateVectorArray( 0 );
	}

	if ( PlLookupHashTableUserData( entityClassLookup, definition->name, strlen( definition->name ) ) != NULL )
	{
		ape_warning_( "Attempted to register a duplicate entity class (%s)\n", definition->name );
		return;
	}

	if ( definition->createFunction == nullptr )
	{
		ape_warning_( "Encountered a class (%s) with no create callback!\n", definition->name );
		return;
	}

	PlInsertHashTableNode( entityClassLookup, definition->name, strlen( definition->name ), ( void * ) definition );
	PlPushBackVectorArrayElement( entityClasses, ( void * ) definition );

	// call the cache function, so we can load resources into memory
	if ( definition->cacheFunction != NULL )
	{
		definition->cacheFunction();
	}
}

const ApeEntityClassDefinition **ape_entity_get_classes( unsigned int *numClasses )
{
	return ( const ApeEntityClassDefinition ** ) PlGetVectorArrayDataEx( entityClasses, numClasses );
}

const ApeEntityClassDefinition *ape_get_entity_class_table( const char *className )
{
	return ( const ApeEntityClassDefinition * ) PlLookupHashTableUserData( entityClassLookup, className, strlen( className ) );
}

ApeEntity *ape_entity_create( ApeWorldNode *parent, const char *className, const char *name, AcmBranch *properties, const PLVector3 *position, const PLVector3 *angles )
{
	const ApeEntityClassDefinition *classDefinition = ape_get_entity_class_table( className );
	if ( classDefinition == NULL )
	{
		ape_warning_( "Failed to find entity class (%s)!\n", className );
		return nullptr;
	}

	ApeEntity *entity = PL_NEW( ApeEntity );
	ape_world_node_setup_( &entity->base, parent, APE_WORLD_NODE_TYPE_ENTITY, name, position, angles );
	entity->classDefinition = classDefinition;
	entity->componentTable  = PlCreateHashTable();

	entity->classData = classDefinition->createFunction( entity, properties );
	if ( entity->classData == nullptr )
	{
		ape_warning_( "Creation failed for entity (%s)!\n", entity->classDefinition->name );
		ape_world_node_destroy( APE_WORLD_NODE( entity ) );
		return nullptr;
	}

	ApeWorldNode *rootNode = ape_world_node_get_root( parent );
	if ( rootNode != nullptr && rootNode->type == APE_WORLD_NODE_TYPE_ROOT )
	{
		ApeWorld *world       = ( ApeWorld * ) rootNode;
		entity->worldListNode = PlInsertLinkedListNode( world->entities, entity );
	}

	return entity;
}

void ape_entity_destroy_( void *data, ApeWorldNode *parent )
{
	ApeEntity *self = data;
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

	PlDestroyLinkedListNode( self->worldListNode );

	PL_DELETE( self );
}

void ape_entity_spawn( ApeEntity *self )
{
	assert( self->classDefinition != NULL );
	if ( self->classDefinition->spawnFunction == NULL )
	{
		return;
	}

	self->classDefinition->spawnFunction( self );
}

void ape_entity_tick( ApeEntity *self )
{
	assert( self->classDefinition != NULL );
	if ( self->classDefinition->tickFunction == NULL )
	{
		return;
	}

	self->classDefinition->tickFunction( self );
}

void ape_entity_draw( ApeEntity *self, ApeLight *light, int flags )
{
	assert( self->classDefinition != NULL );
	if ( self->classDefinition->drawFunction == NULL )
	{
		return;
	}

	self->classDefinition->drawFunction( self, light, flags );
}

void ape_register_entity_component( const ApeEntityComponentDefinition *definition )
{
	if ( entityComponentDefinitions == NULL )
	{
		entityComponentDefinitions = PlCreateHashTable();
	}

	if ( PlLookupHashTableUserData( entityComponentDefinitions, definition->name, strlen( definition->name ) ) != NULL )
	{
		ape_warning_( "Attempted to register a duplicate entity component (%s)\n", definition->name );
		return;
	}

	PlInsertHashTableNode( entityComponentDefinitions, definition->name, strlen( definition->name ), ( void * ) definition );
}

void *ape_entity_add_component( ApeEntity *self, const char *name )
{
	const ApeEntityComponentDefinition *componentDefinition = PlLookupHashTableUserData( entityComponentDefinitions, name, strlen( name ) );
	if ( componentDefinition == NULL )
	{
		ape_warning_( "Failed to find entity component (%s)!\n", name );
		return NULL;
	}

	ApeEntityComponent *component = PL_NEW( ApeEntityComponent );
	if ( componentDefinition->Create != NULL )
	{
		component->data = componentDefinition->Create();
	}

	if ( !PlInsertHashTableNode( self->componentTable, name, strlen( name ), component ) )
	{
		ape_warning_( "Failed to insert entity component (%s): %s\n", name, PlGetError() );

		if ( componentDefinition->Destroy != NULL )
		{
			componentDefinition->Destroy( component );
		}

		PL_DELETE( component );
		return NULL;
	}

	return component->data;
}

void *ape_entity_get_component( ApeEntity *self, const char *name )
{
	return PlLookupHashTableUserData( self->componentTable, name, strlen( name ) );
}

static AcmBranch *serialize_entity( void *self, AcmBranch *root )
{
	ApeEntity *entity = self;
	acm_push_string( root, "className", entity->classDefinition->name, false );

	const ApeEntityClassDefinition *classDefinition = entity->classDefinition;
	if ( classDefinition->serializeFunction != nullptr )
	{
		classDefinition->serializeFunction( entity );
	}

	return root;
}

static ApeWorldNode *deserialize_entity( ApeWorldNode *parent, AcmBranch *root )
{
	const char *className = acm_get_string( root, "className", nullptr );
	if ( className == nullptr )
	{
		ape_warning_( "Failed to deserialize entity: no class name!\n" );
		return nullptr;
	}

	const ApeEntityClassDefinition *classDefinition = ape_get_entity_class_table( className );
	if ( classDefinition == nullptr )
	{
		ape_warning_( "Failed to deserialize entity: class (%s) not found!\n", className );
		return nullptr;
	}

	ApeEntity *entity = ape_entity_create( parent, className, "", nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );

	if ( classDefinition->deserializeFunction != nullptr )
	{
		classDefinition->deserializeFunction( entity, root );
	}

	return APE_WORLD_NODE( entity );
}

const ApeWorldNodeClass ape_entityClass = {
        .identifier          = "entity",
        .magic               = PL_MAGIC_TO_NUM( 'E', 'N', 'T', ' ' ),
        .destroyFunction     = ape_entity_destroy_,
        .serializeFunction   = serialize_entity,
        .deserializeFunction = deserialize_entity,

#if !defined( APE_NO_EDITOR )

        .editorIcon = "resources/new_entity.gif",

#endif
};
