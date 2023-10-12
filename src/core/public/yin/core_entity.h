// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_linkedlist.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

typedef struct AclEntityClassDefinition AclEntityClassDefinition;
typedef struct ApeEntityComponentDefinition ApeEntityComponentDefinition;

#define ACL_ENTITY_MAX_NAME 32

#define ACL_ENTITY_CLASS_BASE_MAGIC PL_MAGIC_TO_NUM( 'B', 'A', 'S', 'E' )

/**
 * Foundation that should be used for all classes.
 */
typedef struct AclEntityClassBase
{
	unsigned int magic;// 'BASE'
} AclEntityClassBase;

typedef struct ApeEntity
{
	char name[ ACL_ENTITY_MAX_NAME ];               // identifier
	const AclEntityClassDefinition *classDefinition;// class that the actor is derived from
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

typedef struct AclEntityClassDefinition
{
	const char *name;

	ApeEntityProperty *propertyList;
	unsigned int numProperties;

	void ( *Cache )( void );// called upon registration
	void *( *Create )( ApeEntity *self, NdBranch *properties );
	void ( *Destroy )( ApeEntity *self );
	void ( *Spawn )( ApeEntity *self );
	void ( *Tick )( ApeEntity *self );
	void ( *Draw )( ApeEntity *self );

	NdBranch *( *Serialize )( ApeEntity *self );
	void ( *Deserialize )( ApeEntity *self, NdBranch *root );
} AclEntityClassDefinition;

typedef const AclEntityClassDefinition *( *ApeEntityClassRegisterFunction )( void );

void acl_entity_register_class( const AclEntityClassDefinition *definition );
const AclEntityClassDefinition *apeGetEntityClassTable( const char *className );

ApeEntity *apeCreateEntity( const char *className, NdBranch *properties );
void apeDestroyEntity( ApeEntity *entity );

void apeTickEntity( ApeEntity *entity );
void apeDrawEntity( ApeEntity *entity );

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

void apeRegisterEntityComponent( const ApeEntityComponentDefinition *definition );
void *apeAddEntityComponent( ApeEntity *entity, const char *name );
void *apeGetEntityComponent( ApeEntity *entity, const char *name );

PL_EXTERN_C_END
