// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_linkedlist.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

typedef struct SS_Acl_EntityClassDefinition SS_Acl_EntityClassDefinition;
typedef struct SS_Acl_EntityComponentDefinition SS_Acl_EntityComponentDefinition;

#define ACL_ENTITY_MAX_NAME 32

typedef struct SS_Acl_Entity
{
	char name[ ACL_ENTITY_MAX_NAME ];                   // identifier
	const SS_Acl_EntityClassDefinition *classDefinition;// class that the actor is derived from
	void *classData;                                    // pointer to the unique data of the class
	struct PLHashTable *componentTable;                 // list of components
	struct ApeWorld *world;                             // world the entity is attached to
} SS_Acl_Entity;

typedef struct SS_Acl_EntityComponent
{
	const SS_Acl_EntityComponentDefinition *componentDefinition;
	void *data;
} SS_Acl_EntityComponent;

typedef struct SS_Acl_EntityProperty
{
	const char *name;
	const char *description;
	void *var;
	PLVariableType type;
} SS_Acl_EntityProperty;

typedef struct SS_Acl_EntityClassDefinition
{
	const char *name;

	SS_Acl_EntityProperty *propertyList;
	unsigned int numProperties;

	void ( *Cache )( void );// called upon registration
	void *( *Create )( SS_Acl_Entity *self, NdBranch *properties );
	void ( *Destroy )( SS_Acl_Entity *self );
	void ( *Spawn )( SS_Acl_Entity *self );
	void ( *Tick )( SS_Acl_Entity *self );
	void ( *Draw )( SS_Acl_Entity *self );

	NdBranch *( *Serialize )( SS_Acl_Entity *self );
	void ( *Deserialize )( SS_Acl_Entity *self, NdBranch *root );
} SS_Acl_EntityClassDefinition;

typedef const SS_Acl_EntityClassDefinition *( *SS_Acl_EntityClassRegisterFunction )( void );

void ss_acl_register_entity_class( const SS_Acl_EntityClassDefinition *definition );
const SS_Acl_EntityClassDefinition *ss_acl_get_entity_class_table( const char *className );

SS_Acl_Entity *ss_acl_entity_create( const char *className, NdBranch *properties );
void ss_acl_entity_destroy( SS_Acl_Entity *entity );

void ss_acl_entity_tick( SS_Acl_Entity *entity );
void ss_acl_entity_draw( SS_Acl_Entity *entity );

////////////////////////////////////////////////////////////////////
// Components

typedef struct SS_Acl_EntityComponentDefinition
{
	const char *name;

	void *( *Create )( void );// required!!
	void ( *Destroy )( void *data );

	NdBranch *( *Serialize )( void );
	void ( *Deserialize )( NdBranch *root );
} SS_Acl_EntityComponentDefinition;

typedef const SS_Acl_EntityComponentDefinition *( *ApeEntityComponentRegisterFunction )( void );

void ss_acl_register_entity_component( const SS_Acl_EntityComponentDefinition *definition );
void *ss_acl_entity_add_component( SS_Acl_Entity *entity, const char *name );
void *ss_acl_entity_get_component( SS_Acl_Entity *entity, const char *name );

PL_EXTERN_C_END
