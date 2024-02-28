// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_linkedlist.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

typedef struct ApeEntityClassDefinition ApeEntityClassDefinition;
typedef struct ApeEntityComponentDefinition ApeEntityComponentDefinition;

#define APE_ENTITY_MAX_NAME 32

typedef struct ApeEntity
{
	char name[ APE_ENTITY_MAX_NAME ];               // identifier
	const ApeEntityClassDefinition *classDefinition;// class that the actor is derived from
	void *classData;                                // pointer to the unique data of the class
	struct PLHashTable *componentTable;             // list of components
	struct ApeWorld *world;                         // world the entity is attached to
} ApeEntity;

typedef struct ApeEntityComponent
{
	const ApeEntityComponentDefinition *componentDefinition;
	void *data;
} ApeEntityComponent;

typedef struct ApeEntityProperty
{
	const char *name;
	const char *description;
	void *var;
	PLVariableType type;
} ApeEntityProperty;

typedef struct ApeEntityClassDefinition
{
	const char *name;
	const char *description;

	bool excludeInEditor;

	ApeEntityProperty *propertyList;
	unsigned int numProperties;

	void ( *cacheFunction )( void );// called upon registration
	void *( *createFunction )( ApeEntity *self, NdBranch *properties );
	void ( *destroyFunction )( ApeEntity *self );
	void ( *spawnFunction )( ApeEntity *self );
	void ( *tickFunction )( ApeEntity *self );
	void ( *drawFunction )( ApeEntity *self );

	NdBranch *( *serializeFunction )( ApeEntity *self );
	void ( *deserializeFunction )( ApeEntity *self, NdBranch *root );
} ApeEntityClassDefinition;

typedef const ApeEntityClassDefinition *( *SS_Acl_EntityClassRegisterFunction )( void );

void ape_register_entity_class( const ApeEntityClassDefinition *definition );
const ApeEntityClassDefinition *ape_get_entity_class_table( const char *className );

ApeEntity *ape_entity_create( const char *className, NdBranch *properties );
void ape_entity_destroy( ApeEntity *entity );

void ape_entity_tick( ApeEntity *entity );
void ape_entity_draw( ApeEntity *entity );

////////////////////////////////////////////////////////////////////
// Components

typedef struct ApeEntityComponentDefinition
{
	const char *name;

	void *( *Create )( void );// required!!
	void ( *Destroy )( void *data );

	NdBranch *( *Serialize )( void );
	void ( *Deserialize )( NdBranch *root );
} ApeEntityComponentDefinition;

typedef const ApeEntityComponentDefinition *( *ApeEntityComponentRegisterFunction )( void );

void ape_register_entity_component( const ApeEntityComponentDefinition *definition );
void *ss_acl_entity_add_component( ApeEntity *entity, const char *name );
void *ss_acl_entity_get_component( ApeEntity *entity, const char *name );

PL_EXTERN_C_END
