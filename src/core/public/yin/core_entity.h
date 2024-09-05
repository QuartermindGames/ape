// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_linkedlist.h>

PL_EXTERN_C

typedef struct AcmBranch AcmBranch;

typedef struct ApeEntityClassDefinition     ApeEntityClassDefinition;
typedef struct ApeEntityComponentDefinition ApeEntityComponentDefinition;

#define APE_ENTITY_MAX_NAME 32

typedef struct ApeEntity
{
	// This should always come first!
	ApeWorldNode base;

	char                            name[ APE_ENTITY_MAX_NAME ];// identifier
	const ApeEntityClassDefinition *classDefinition;            // class that the actor is derived from
	void                           *classData;                  // pointer to the unique data of the class
	struct PLHashTable             *componentTable;             // list of components
	struct ApeWorld                *world;                      // world the entity is attached to
} ApeEntity;

typedef struct ApeEntityComponent
{
	const ApeEntityComponentDefinition *componentDefinition;
	void                               *data;
} ApeEntityComponent;

#define APE_ENT_CLASS( SELF, TYPE )     ( ( TYPE * ) ( ( SELF )->classData ) )
#define APE_ENT_COMPONENT( SELF, TYPE ) ( ( TYPE * ) ( ( SELF )->data ) )

typedef struct ApeEntityProperty
{
	const char    *name;
	const char    *description;
	void          *var;
	PLVariableType type;
} ApeEntityProperty;

typedef struct ApeEntityClassDefinition
{
	const char *name;       // general identifier
	const char *description;// for the editor

	bool excludeInEditor;

	ApeEntityProperty *propertyList;
	unsigned int       numProperties;

	void ( *cacheFunction )( void );                                    // called upon registration
	void *( *createFunction )( ApeEntity *self, AcmBranch *properties );// *required* called upon entity allocation, this is when the class should be allocated and returned
	void ( *destroyFunction )( ApeEntity *self );                       // called when the entity is free'd, which should be done for the class too
	void ( *spawnFunction )( ApeEntity *self );                         // this gets called when the entity is actually spawned into the world, at which point the class state can be reset
	void ( *tickFunction )( ApeEntity *self );                          // called per ticket, allowing for behaviours
	void ( *drawFunction )( ApeEntity *self );

	AcmBranch *( *serializeFunction )( ApeEntity *self );
	void ( *deserializeFunction )( ApeEntity *self, AcmBranch *root );
} ApeEntityClassDefinition;

typedef const ApeEntityClassDefinition *( *SS_Acl_EntityClassRegisterFunction )( void );

void                            ape_register_entity_class( const ApeEntityClassDefinition *definition );
const ApeEntityClassDefinition *ape_get_entity_class_table( const char *className );

ApeEntity *ape_create_entity( const char *className, AcmBranch *properties, ApeWorldNode *parent );

void ape_entity_tick( ApeEntity *self );
void ape_entity_draw( ApeEntity *self );

////////////////////////////////////////////////////////////////////
// Components

typedef struct ApeEntityComponentDefinition
{
	const char *name;

	void *( *Create )( void );// required!!
	void ( *Destroy )( void *data );

	AcmBranch *( *Serialize )( void );
	void ( *Deserialize )( AcmBranch *root );
} ApeEntityComponentDefinition;

typedef const ApeEntityComponentDefinition *( *ApeEntityComponentRegisterFunction )( void );

void  ape_register_entity_component( const ApeEntityComponentDefinition *definition );
void *ape_entity_add_component( ApeEntity *self, const char *name );
void *ape_entity_get_component( ApeEntity *self, const char *name );

PL_EXTERN_C_END
