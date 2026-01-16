// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "node_entity.h"

#include "editor/editor.h"
#include "world/world.h"

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

	ape_console_print_( "Listing %u entity classes...\n", PlGetNumHashTableNodes( entityClassLookup ) );

	PLHashTableNode *node = PlGetFirstHashTableNode( entityClassLookup );
	while ( node != NULL )
	{
		ApeEntityClassDefinition *classDefinition = PlGetHashTableNodeUserData( node );
		ape_console_print_( "-------------------------------------------------\n" );
		ape_console_print_( "%s : %s\n", classDefinition->name, classDefinition->description != NULL ? classDefinition->description : "none" );
		ape_console_print_( " num properties       = %u\n", classDefinition->numProperties );
		ape_console_print_( " cache callback       = %p\n", classDefinition->cacheFunction );
		ape_console_print_( " create callback      = %p\n", classDefinition->createFunction );
		ape_console_print_( " destroy callback     = %p\n", classDefinition->destroyFunction );
		ape_console_print_( " spawn callback       = %p\n", classDefinition->spawnFunction );
		ape_console_print_( " tick callback        = %p\n", classDefinition->tickFunction );
		ape_console_print_( " draw callback        = %p\n", classDefinition->drawFunction );
		ape_console_print_( " serialize callback   = %p\n", classDefinition->serializeFunction );
		ape_console_print_( " deserialize callback = %p\n", classDefinition->deserializeFunction );

		node = PlGetNextHashTableNode( node );
	}

	ape_console_print_( "\nListing %u entity components...\n", PlGetNumHashTableNodes( entityComponentDefinitions ) );

	node = PlGetFirstHashTableNode( entityComponentDefinitions );
	while ( node != NULL )
	{
		ApeEntityComponentDefinition *componentDefinition = PlGetHashTableNodeUserData( node );
		ape_console_print_( "-------------------------------------------------\n" );
		ape_console_print_( "%s\n", componentDefinition->name );

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
		ape_console_warning_( "Attempted to register a duplicate entity class (%s)\n", definition->name );
		return;
	}

	if ( definition->createFunction == nullptr )
	{
		ape_console_warning_( "Encountered a class (%s) with no create callback!\n", definition->name );
		return;
	}

	assert( ape_editor_validate_properties_( definition->properties, definition->numProperties ) );

	PlInsertHashTableNode( entityClassLookup, definition->name, strlen( definition->name ), ( void * ) definition );
	PlPushBackVectorArrayElement( entityClasses, ( void * ) definition );

	// call the cache function, so we can load resources into memory
	if ( definition->cacheFunction != NULL )
	{
		definition->cacheFunction();
	}

	ape_console_print_( "Registered \"%s\" entity class\n", definition->name );
}

const ApeEntityClassDefinition **ape_entity_get_classes( unsigned int *numClasses )
{
	return ( const ApeEntityClassDefinition ** ) PlGetVectorArrayDataEx( entityClasses, numClasses );
}

const ApeEntityClassDefinition *ape_get_entity_class_table( const char *className )
{
	return ( const ApeEntityClassDefinition * ) PlLookupHashTableUserData( entityClassLookup, className, strlen( className ) );
}

ApeEntity *ape_entity_create( ApeWorldNode *parent, const char *className, const char *name, AcmBranch *properties, const QmMathVector3f *position, const QmMathVector3f *angles )
{
	const ApeEntityClassDefinition *classDefinition = ape_get_entity_class_table( className );
	if ( classDefinition == NULL )
	{
		ape_console_warning_( "Failed to find entity class (%s)!\n", className );
		return nullptr;
	}

	ApeEntity *entity = QM_OS_MEMORY_NEW( ApeEntity );
	ape_world_node_setup_( &entity->base, parent, APE_WORLD_NODE_TYPE_ENTITY, name, position, angles );
	entity->classDefinition = classDefinition;
	entity->componentTable  = PlCreateHashTable();

	entity->classData = classDefinition->createFunction( entity, properties );
	if ( entity->classData == nullptr )
	{
		ape_console_warning_( "Creation failed for entity (%s)!\n", entity->classDefinition->name );
		ape_world_node_destroy( APE_WORLD_NODE( entity ) );
		return nullptr;
	}

	ApeWorldNode *rootNode = ape_world_node_get_root( parent );
	if ( rootNode != nullptr && rootNode->type == APE_WORLD_NODE_TYPE_ROOT )
	{
		ApeWorld *world       = ( ApeWorld * ) rootNode;
		entity->worldListNode = PlInsertLinkedListNode( world->entities, entity );
	}

#if !defined( APE_NO_EDITOR )
	const char *editorSpritePath = classDefinition->editorSpritePath;
	if ( editorSpritePath != nullptr )
	{
		entity->editorSprite = ape_material_cache( editorSpritePath, APE_CACHE_GROUP_EDITOR, true );
	}
#endif

	return entity;
}

void ape_entity_destroy_( void *data, ApeWorldNode *parent )
{
	ApeEntity *self = data;
	if ( self == nullptr )
	{
		return;
	}

	ApeEntityComponent *component;
	COM_ITERATE_HASHED_LIST( component, self->componentTable, i )
	{
		if ( component->data != nullptr && component->componentDefinition->destroyFunction != nullptr )
		{
			component->componentDefinition->destroyFunction( component->data );
		}

		qm_os_memory_free( component );
	}

	if ( self->classDefinition->destroyFunction != nullptr )
	{
		self->classDefinition->destroyFunction( self );
	}

	if ( self->editorSprite != nullptr )
	{
		ape_material_release( self->editorSprite );
	}

	PlDestroyHashTable( self->componentTable );

	PlDestroyLinkedListNode( self->worldListNode );

	qm_os_memory_free( self );
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

void ape_entity_tick( ApeEntity *self, double delta )
{
	assert( self->classDefinition != NULL );
	if ( self->classDefinition->tickFunction == NULL )
	{
		return;
	}

	self->classDefinition->tickFunction( self, delta );
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
		ape_console_warning_( "Attempted to register a duplicate entity component (%s)\n", definition->name );
		return;
	}

	assert( definition->createFunction != nullptr );
	assert( definition->destroyFunction != nullptr );

	assert( ape_editor_validate_properties_( definition->properties, definition->numProperties ) );

	PlInsertHashTableNode( entityComponentDefinitions, definition->name, strlen( definition->name ), ( void * ) definition );

	ape_console_print_( "Registered \"%s\" entity component\n", definition->name );
}

void *ape_entity_add_component( ApeEntity *self, const char *name )
{
	const ApeEntityComponentDefinition *componentDefinition = PlLookupHashTableUserData( entityComponentDefinitions, name, strlen( name ) );
	if ( componentDefinition == NULL )
	{
		ape_console_warning_( "Failed to find entity component (%s)!\n", name );
		return NULL;
	}

	ApeEntityComponent *component  = QM_OS_MEMORY_NEW( ApeEntityComponent );
	component->componentDefinition = componentDefinition;
	if ( component->componentDefinition->createFunction != NULL )
	{
		component->data = component->componentDefinition->createFunction();
	}

	if ( !PlInsertHashTableNode( self->componentTable, name, strlen( name ), component ) )
	{
		ape_console_warning_( "Failed to insert entity component (%s): %s\n", name, PlGetError() );

		if ( component->componentDefinition->destroyFunction != NULL )
		{
			component->componentDefinition->destroyFunction( component );
		}

		qm_os_memory_free( component );
		return NULL;
	}

	return component->data;
}

void *ape_entity_get_component( ApeEntity *self, const char *name )
{
	ApeEntityComponent *component = PlLookupHashTableUserData( self->componentTable, name, strlen( name ) );
	if ( component == nullptr )
	{
		return nullptr;
	}

	return component->data;
}

static void serialize_properties( AcmBranch *root, const ApeProperty *properties, const unsigned int numProperties, const uintptr_t baseOffset )
{
	for ( unsigned int i = 0; i < numProperties; ++i )
	{
		const ApeProperty *property = &properties[ i ];

		void *ptr = ( char * ) baseOffset + property->offset;
		switch ( property->type )
		{
			default:
				ape_console_error_( false, "Failed to serialize property type (%u)!\n", property->type );
				break;
			case APE_PROPERTY_TYPE_FLOAT:
				acm_push_f32( root, property->internalName, *( ApeFloatProperty * ) ptr );
				break;
			case APE_PROPERTY_TYPE_VEC2:
				com_acm_push_vector2( root, property->internalName, ptr, true );
				break;
			case APE_PROPERTY_TYPE_VEC3:
				com_acm_push_vector3( root, property->internalName, ptr, true );
				break;
			case APE_PROPERTY_TYPE_VEC4:
				com_acm_push_vector4( root, property->internalName, ptr, true );
				break;
			case APE_PROPERTY_TYPE_COLOUR:
				com_acm_push_colour4f( root, property->internalName, ptr, true );
				break;
			case APE_PROPERTY_TYPE_ENUM:
				acm_push_ui32( root, property->internalName, *( ApeEnumProperty * ) ptr );
				break;
			case APE_PROPERTY_TYPE_INTEGER:
				acm_push_i32( root, property->internalName, *( ApeIntegerProperty * ) ptr );
				break;
			case APE_PROPERTY_TYPE_STRING:
			case APE_PROPERTY_TYPE_PATH:
				acm_push_string( root, property->internalName, ptr, true );
				break;
			case APE_PROPERTY_TYPE_BOOLEAN:
				acm_push_bool( root, property->internalName, *( ApeBooleanProperty * ) ptr );
				break;
		}
	}
}

static void deserialize_properties( AcmBranch *root, const ApeProperty *properties, const unsigned int numProperties, const uintptr_t baseOffset )
{
	for ( unsigned int i = 0; i < numProperties; ++i )
	{
		const ApeProperty *property = &properties[ i ];

		void *ptr = ( char * ) baseOffset + property->offset;
		switch ( property->type )
		{
			default:
				ape_console_error_( false, "Failed to deserialize property type (%u)!\n", property->type );
				break;
			case APE_PROPERTY_TYPE_FLOAT:
				*( ApeFloatProperty * ) ptr = acm_get_f32( root, property->internalName, *( float * ) ptr );
				break;
			case APE_PROPERTY_TYPE_VEC2:
				*( ApeVec2Property * ) ptr = com_acm_get_vector2( root, property->internalName, ptr );
				break;
			case APE_PROPERTY_TYPE_VEC3:
				*( ApeVec3Property * ) ptr = com_acm_get_vector3( root, property->internalName, ptr );
				break;
			case APE_PROPERTY_TYPE_VEC4:
				*( ApeVec4Property * ) ptr = com_acm_get_vector4( root, property->internalName, ptr );
				break;
			case APE_PROPERTY_TYPE_COLOUR:
				*( ApeColour4fProperty * ) ptr = com_acm_get_colour_f32( root, property->internalName, ptr );
				break;
			case APE_PROPERTY_TYPE_ENUM:
				*( ApeEnumProperty * ) ptr = acm_get_uint( root, property->internalName, *( ApeEnumProperty * ) ptr );
				break;
			case APE_PROPERTY_TYPE_INTEGER:
				*( ApeIntegerProperty * ) ptr = acm_get_int( root, property->internalName, *( ApeIntegerProperty * ) ptr );
				break;
			case APE_PROPERTY_TYPE_STRING:
			case APE_PROPERTY_TYPE_PATH:
			{
				const char *str = acm_get_string( root, property->internalName, ptr );
				snprintf( ptr, property->stringType.maxSize, "%s", str );
				break;
			}
			case APE_PROPERTY_TYPE_BOOLEAN:
				*( ApeBooleanProperty * ) ptr = acm_get_bool( root, property->internalName, *( bool * ) ptr );
				break;
		}
	}
}

static AcmBranch *serialize_entity( void *self, AcmBranch *root )
{
	ApeEntity *entity = self;
	acm_push_string( root, "className", entity->classDefinition->name, false );

	const ApeEntityClassDefinition *classDefinition = entity->classDefinition;
	assert( classDefinition != nullptr );

	serialize_properties( root, classDefinition->properties, classDefinition->numProperties, ( intptr_t ) entity->classData );

	// call this after to allow for any special logic we might want to envoke,
	// for instance there might be some versioning handling or other nonsense
	if ( classDefinition->serializeFunction != nullptr )
	{
		classDefinition->serializeFunction( entity, root );
	}

	// attempt to serialize components
	if ( PlGetNumHashTableNodes( entity->componentTable ) > 0 )
	{
		AcmBranch          *branch = acm_push_array_object( root, "components" );
		ApeEntityComponent *component;
		COM_ITERATE_HASHED_LIST( component, entity->componentTable, i )
		{
			const ApeEntityComponentDefinition *componentDefinition = component->componentDefinition;

			AcmBranch *componentBranch = acm_push_object( branch, componentDefinition->name );
			acm_push_string( componentBranch, "name", componentDefinition->name, false );

			serialize_properties( componentBranch, componentDefinition->properties, componentDefinition->numProperties, ( intptr_t ) component->data );

			// call this after to allow for any special logic we might want to envoke,
			// for instance there might be some versioning handling or other nonsense
			if ( componentDefinition->serializeFunction != nullptr )
			{
				componentDefinition->serializeFunction( component->data, componentBranch );
			}
		}
	}

	return root;
}

static ApeWorldNode *deserialize_entity( ApeWorldNode *parent, AcmBranch *root )
{
	const char *className = acm_get_string( root, "className", nullptr );
	if ( className == nullptr )
	{
		ape_console_warning_( "Failed to deserialize entity: no class name!\n" );
		return nullptr;
	}

	const ApeEntityClassDefinition *classDefinition = ape_get_entity_class_table( className );
	if ( classDefinition == nullptr )
	{
		ape_console_warning_( "Failed to deserialize entity: class (%s) not found!\n", className );
		return nullptr;
	}

	ApeEntity *entity = ape_entity_create( parent, className, "", nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );
	if ( entity == nullptr )
	{
		ape_console_warning_( "Failed to deserialize entity: entity create failed!\n" );
		return nullptr;
	}

	// for deserialization, it's going to be wiser to do this before we start
	// deserializing the class as it'll probably want to look these up!
	AcmBranch *branch;
	if ( ( branch = acm_get_child_by_name( root, "components" ) ) != nullptr )
	{
		ACM_ITERATE_BRANCH( branch, i )
		{
			const char *name = acm_get_string( i, "name", nullptr );
			if ( name == nullptr )
			{
				ape_console_warning_( "No name provided for entity component!\n" );
				continue;
			}

			const ApeEntityComponentDefinition *componentDefinition = PlLookupHashTableUserData( entityComponentDefinitions, name, strlen( name ) );
			if ( componentDefinition == NULL )
			{
				ape_console_warning_( "Failed to find entity component (%s)!\n", name );
				continue;
			}

			void *component = ape_entity_add_component( entity, name );

			deserialize_properties( i, componentDefinition->properties, componentDefinition->numProperties, ( uintptr_t ) component );

			// call this after to allow for any special logic we might want to envoke,
			// for instance there might be some versioning handling or other nonsense
			if ( componentDefinition->deserializeFunction != nullptr )
			{
				componentDefinition->deserializeFunction( component, i );
			}
		}
	}

	deserialize_properties( root, classDefinition->properties, classDefinition->numProperties, ( uintptr_t ) entity->classData );

	// call this after to allow for any special logic we might want to envoke,
	// for instance there might be some versioning handling or other nonsense
	if ( classDefinition->deserializeFunction != nullptr )
	{
		classDefinition->deserializeFunction( entity, root );
	}

	return APE_WORLD_NODE( entity );
}

static void ape_entity_draw_editor_( void *self, const bool isSelected )
{
	ApeEntity *entity = self;

	const ApeEntityClassDefinition *classDefinition = entity->classDefinition;
	assert( classDefinition != nullptr );

	if ( classDefinition->onDrawEditor == nullptr )
	{
		return;
	}

	classDefinition->onDrawEditor( entity, isSelected );
}

const ApeWorldNodeClass ape_entityClass = {
        .identifier  = "entity",
        .magic       = QM_OS_MAGIC_TO_NUM( 'E', 'N', 'T', ' ' ),
        .destroy     = ape_entity_destroy_,
        .serialize   = serialize_entity,
        .deserialize = deserialize_entity,

#if !defined( APE_NO_EDITOR )

        .editorIcon = "resources/new_entity.gif",

        .onDrawEditor = ape_entity_draw_editor_,

#endif
};
