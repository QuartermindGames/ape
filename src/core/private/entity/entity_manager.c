// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// A manager for entities and their components

#include <plcore/pl_hashtable.h>

#include <yin/node.h>

#include "../core_private.h"

#include "entity.h"

static PLLinkedList *entityList = NULL;

static PLHashTable *entityPrefabTable   = NULL;
static PLHashTable *componentSpawnTable = NULL;

static void TestCommand( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	YNCoreEntity *entity = YnCore_EntityManager_CreateEntity();
	if ( entity == NULL )
	{
		PRINT_WARNING( "Failed to create entity!\n" );
		return;
	}

	if ( YnCore_Entity_AttachComponentByName( entity, "transform" ) == NULL )
		PRINT_WARNING( "Failed to attach \"transform\" component to entity!\n" );

	YnCore_EntityManager_DestroyEntity( entity );
}

void ogeEntityManager_Initialize( void )
{
	PlRegisterConsoleCommand( "entity.test", "Test the entity system.", 0, TestCommand );

	entityList = PlCreateLinkedList();
	if ( entityList == NULL )
		PRINT_ERROR( "Failed to create entity list: %s\n", PlGetError() );

	entityPrefabTable = PlCreateHashTable();
	if ( entityPrefabTable == NULL )
		PRINT_ERROR( "Failed to create entity prefab list: %s\n", PlGetError() );

	componentSpawnTable = PlCreateHashTable();
	if ( componentSpawnTable == NULL )
		PRINT_ERROR( "Failed to create entity component list: %s\n", PlGetError() );

	PRINT( "Entity Manager initialized\n" );
}

void ogeEntityManager_Shutdown( void )
{
	// Clear up all entities first
	if ( entityList != NULL )
	{
		PLLinkedListNode *node = PlGetFirstNode( entityList );
		while ( node != NULL )
		{
			YNCoreEntity *entity = PlGetLinkedListNodeUserData( node );
			node                 = PlGetNextLinkedListNode( node );
			YnCore_EntityManager_DestroyEntity( entity );
		}
		PlDestroyLinkedList( entityList );
	}

	// Clear up the base components
	PlDestroyHashTable( componentSpawnTable );

	// Clear up entity prefabs
	if ( entityPrefabTable != NULL )
	{
		PLHashTableNode *hashNode = PlGetFirstHashTableNode( entityPrefabTable );
		while ( hashNode != NULL )
		{
			YNCoreEntityPrefab *template = PlGetHashTableNodeUserData( hashNode );

			// Iterate over and clear out all the component references
			for ( unsigned int i = 0; i < template->numComponents; ++i )
				ndDestroyBranch( template->components[ i ].properties );
			PL_DELETE( template->components );

			PL_DELETE( template );
			hashNode = PlGetNextHashTableNode( entityPrefabTable, hashNode );
		}
		PlDestroyHashTable( entityPrefabTable );
	}
}

static void IterateEntities( void ( *callbackHandler )( YNCoreEntityComponent *component, YNCoreEntityComponentBase *componentTemplate, void *user ), void *user )
{
	PLHashTableNode *node = PlGetFirstHashTableNode( componentSpawnTable );
	while ( node != NULL )
	{
		YNCoreEntityComponentBase *componentTemplate = PlGetHashTableNodeUserData( node );
		if ( componentTemplate->activeComponents != NULL )
		{
			PLLinkedListNode *subNode = PlGetFirstNode( componentTemplate->activeComponents );
			while ( subNode != NULL )
			{
				YNCoreEntityComponent *component = PlGetLinkedListNodeUserData( subNode );
				subNode                          = PlGetNextLinkedListNode( subNode );
				callbackHandler( component, componentTemplate, user );
			}
		}
		node = PlGetNextHashTableNode( componentSpawnTable, node );
	}
}

static void CallEntityTick( YNCoreEntityComponent *component, YNCoreEntityComponentBase *base, PL_UNUSED void *user )
{
	if ( base->callbackTable->tickFunction == NULL )
		return;

	base->callbackTable->tickFunction( component );
}

static void CallEntityDraw( YNCoreEntityComponent *component, YNCoreEntityComponentBase *componentTemplate, PL_UNUSED void *user )
{
	if ( componentTemplate->callbackTable->drawFunction == NULL )
		return;

	componentTemplate->callbackTable->drawFunction( component );
}

static void SerializeEntityCallback( YNCoreEntityComponent *component, YNCoreEntityComponentBase *componentTemplate, void *user )
{
	if ( componentTemplate->callbackTable->serializeFunction == NULL )
		return;

	componentTemplate->callbackTable->serializeFunction( component, ( NdBranch * ) user );
}

static void DeserializeEntityCallback( YNCoreEntityComponent *component, YNCoreEntityComponentBase *componentTemplate, void *user )
{
	if ( componentTemplate->callbackTable->deserializeFunction == NULL )
		return;

	componentTemplate->callbackTable->deserializeFunction( component, ( NdBranch * ) user );
}

void ogeEntityManager_Tick( void )
{
	IterateEntities( CallEntityTick, NULL );
}

void ogeEntityManager_Draw( OgeCamera *camera, OgeWorldSector *sector )
{
	IterateEntities( CallEntityDraw, NULL );
}

void YnCore_EntityManager_Save( NdBranch *root )
{
	IterateEntities( SerializeEntityCallback, root );
}

void ogeEntityManager_Restore( NdBranch *root )
{
	IterateEntities( DeserializeEntityCallback, root );
}

unsigned int YnCore_EntityManager_GetNumOfEntities( void )
{
	return PlGetNumLinkedListNodes( entityList );
}

/////////////////////////////////////////////////////////////////
// Entity Prefabs

static YNCoreEntityPrefab *ParseEntityPrefab( const char *path, NdBranch *root )
{
	const char *str;
	str = ndGetStringByName( root, "name", NULL );
	assert( str != NULL );
	if ( str == NULL )
	{
		PRINT_WARNING( "No valid name provided for entity template, \"%s\"!\n", path );
		return NULL;
	}

	NdBranch *node;
	node = ndGetChildByName( root, "components" );
	assert( node != NULL );
	if ( node == NULL )
	{
		PRINT_WARNING( "No components for entity template, \"%s\"!\n", path );
		return NULL;
	}

	YNCoreEntityPrefab *prefab = PL_NEW( YNCoreEntityPrefab );

	snprintf( prefab->name, sizeof( prefab->name ), "%s", str );

	// description is a field we'll display in the editor,
	// just essentially an explanation of what the entity does
	str = ndGetStringByName( root, "description", NULL );
	if ( str != NULL )
		snprintf( prefab->description, sizeof( prefab->description ), "%s", str );

	prefab->numComponents = ndGetNumOfChildren( node );
	prefab->components    = PL_NEW_( YNCoreEntityPrefabComponent, prefab->numComponents );

	// Get the first child of the components list
	node = ndGetFirstChild( node );
	for ( unsigned int i = 0; i < prefab->numComponents; ++i )
	{
		assert( node != NULL );
		if ( node == NULL )
		{
			PRINT_WARNING( "Encountered an invalid component node, might be a parser error!\n" );
			break;
		}

		const char *name = ndGetStringByName( node, "name", NULL );
		assert( name != NULL );
		if ( name == NULL )
		{
			PRINT_WARNING( "Component listed for prefab without a name!\n" );
			node = ndGetNextChild( node );
			continue;
		}

		const YNCoreEntityComponentBase *base = YnCore_EntityManager_GetComponentBaseByName( name );
		if ( base == NULL )
		{
			PRINT_WARNING( "\"%s\" is not a valid entity component!\n", name );
			node = ndGetNextChild( node );
			continue;
		}

		prefab->components[ i ].base = base;

		// Attempt to fetch the properties so we can hand it over to the component later
		NdBranch *propertiesNode = ndGetChildByName( node, "properties" );
		if ( propertiesNode != NULL )
			prefab->components[ i ].properties = ndCopyBranch( propertiesNode );

		node = ndGetNextChild( node );
	}

	return prefab;
}

void YnCore_EntityManager_RegisterEntityPrefab( const char *path )
{
	NdBranch *root = ndLoadFile( path, "entityPrefab" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Failed to open entity template, \"%s\": %s\n", path, ndGetErrorMessage() );
		return;
	}

	YNCoreEntityPrefab *entityTemplate = ParseEntityPrefab( path, root );
	if ( entityTemplate != NULL )
	{
		if ( PlInsertHashTableNode( entityPrefabTable, entityTemplate->name, strlen( entityTemplate->name ), entityTemplate ) )
			PRINT( "Registered \"%s\" entity (%s)\n", entityTemplate->name, path );
		else
			PRINT_WARNING( "Failed to register entity template: %s\n", PlGetError() );
	}
	else
		PRINT_WARNING( "Failed to register entity template: %s\nSee log for details!\n", path );

	ndDestroyBranch( root );
}

/**
 * Attempts to load the entity template at the specified path and add it to our hash table.
 */
static void RegisterEntityPrefab( const char *path, PL_UNUSED void *userData )
{
	YnCore_EntityManager_RegisterEntityPrefab( path );
}

void ogeEntityManager_RegisterEntityPrefabs( void )
{
	PRINT( "Registering entity prefabs...\n" );

	PlScanDirectory( "entities", "n", RegisterEntityPrefab, true, NULL );
}

const YNCoreEntityPrefab *YnCore_EntityManager_GetPrefabByName( const char *name )
{
	return PlLookupHashTableUserData( entityPrefabTable, name, strlen( name ) );
}

/////////////////////////////////////////////////////////////////
// Entity Components

/**
 * Attempt to find the specified template by name.
 */
const YNCoreEntityComponentBase *YnCore_EntityManager_GetComponentBaseByName( const char *name )
{
	return PlLookupHashTableUserData( componentSpawnTable, name, strlen( name ) );
}

bool ogeEntityManager_RegisterComponent( const char *name, const YNCoreEntityComponentCallbackTable *callbackTable )
{
	// check if it's been registered already
	if ( YnCore_EntityManager_GetComponentBaseByName( name ) != NULL )
	{
		PRINT_WARNING( "Component \"%s\" was already registered!\n", name );
		return false;
	}

	// otherwise, alloc a slot and add it to the list
	YNCoreEntityComponentBase *base = PL_NEW( YNCoreEntityComponentBase );
	snprintf( base->name, sizeof( base->name ), "%s", name );
	base->callbackTable = callbackTable;
	PlInsertHashTableNode( componentSpawnTable, name, strlen( name ), base );
	PRINT( "Registered \"%s\" component\n", base->name );

	return true;
}

YNCoreEntityComponent *YnCore_EntityManager_AddComponentToEntity( YNCoreEntity *entity, const char *name )
{
	const YNCoreEntityComponentBase *componentTemplate = YnCore_EntityManager_GetComponentBaseByName( name );
	if ( componentTemplate == NULL )
	{
		PRINT_WARNING( "Attempted to add an invalid component, \"%s\", onto an entity!\n", name );
		return NULL;
	}

	YNCoreEntityComponent *component = PL_NEW( YNCoreEntityComponent );
	component->entity                = entity;
	component->base                  = componentTemplate;
	component->listNode              = PlInsertLinkedListNode( entity->components, component );

	return component;
}

/////////////////////////////////////////////////////////////////
// Entitys

YNCoreEntity *YnCore_EntityManager_CreateEntity( void )
{
	YNCoreEntity *entity = PL_NEW( YNCoreEntity );
	PlGenerateUniqueIdentifier( entity->name, sizeof( entity->name ) );
	entity->components = PlCreateLinkedList();
	entity->id         = PlGetNumLinkedListNodes( entityList );
	entity->listNode   = PlInsertLinkedListNode( entityList, entity );
	PRINT_DEBUG( "Created entity (%u %s)\n", entity->id, entity->name );
	return entity;
}

/**
 * Creates a new entity and attaches all of the components from the given
 * prefab to that entity.
 * @param name Name of the prefab to lookup.
 * @return NULL on fail, otherwise a pointer to the newly allocated entity.
 */
YNCoreEntity *YnCore_EntityManager_CreateEntityFromPrefab( const char *name )
{
	const YNCoreEntityPrefab *prefab = YnCore_EntityManager_GetPrefabByName( name );
	if ( prefab == NULL )
	{
		PRINT_WARNING( "Failed to create entity by prefab! (%s == NULL)\n", name );
		return NULL;
	}

	YNCoreEntity *entity = YnCore_EntityManager_CreateEntity();

	for ( unsigned int i = 0; i < prefab->numComponents; ++i )
	{
		YNCoreEntityComponent *component = PL_NEW( YNCoreEntityComponent );
		component->entity                = entity;
		component->base                  = prefab->components[ i ].base;
		component->listNode              = PlInsertLinkedListNode( entity->components, component );
	}

	return entity;
}

void YnCore_EntityManager_DestroyEntity( YNCoreEntity *entity )
{
	YnCore_Entity_RemoveAllComponents( entity );
	PlDestroyLinkedList( entity->components );
	PL_DELETE( entity );
}
