// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_linkedlist.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

typedef char ApeEntityClassName[ 64 ];
typedef char ApeEntityName[ 64 ];

typedef struct ApeEntity ApeEntity;
typedef struct ApeEntityPrefab ApeEntityPrefab;
typedef struct ApeEntityComponentBase ApeEntityComponentBase;
typedef struct ApeEntityComponent
{
	const ApeEntityComponentBase *base;
	ApeEntity *entity;
	struct PLLinkedListNode *listNode;
	void *userData;
} ApeEntityComponent;

#define ENTITY_COMPONENT_CAST( SELF, TYPE ) ( ( TYPE * ) ( SELF )->userData )

typedef void ( *ApeECSpawnFunction )( ApeEntityComponent *self );
typedef void ( *ApeECTickFunction )( ApeEntityComponent *self );
typedef void ( *ApeECDrawFunction )( ApeEntityComponent *self );
typedef void ( *ApeECDestroyFunction )( ApeEntityComponent *self );
typedef NdBranch *( *ApeECSerializeFunction )( ApeEntityComponent *self, NdBranch *root );
typedef NdBranch *( *ApeECDeserializeFunction )( ApeEntityComponent *self, NdBranch *root );

typedef struct ApeEntityComponentCallbackTable
{
	ApeECSpawnFunction spawnFunction;
	ApeECTickFunction tickFunction;
	ApeECDrawFunction drawFunction;
	ApeECDestroyFunction destroyFunction;
	ApeECSerializeFunction serializeFunction;
	ApeECDeserializeFunction deserializeFunction;

	const struct ApeEditorField *editorFields;
	unsigned int numEditorFields;
} ApeEntityComponentCallbackTable;

void ogeEntityManager_Initialize( void );
void apeShutdownEntityManager( void );
void apeTickEntityManager( void );
void ogeEntityManager_Draw( struct ApeCamera *camera, struct ApeWorldRoom *sector );
void YnCore_EntityManager_Save( NdBranch *root );
void ogeEntityManager_Restore( NdBranch *root );

// Prefabs
void YnCore_EntityManager_RegisterEntityPrefab( const char *path );
void apeRegisterEntityPrefabs( void );
const ApeEntityPrefab *YnCore_EntityManager_GetPrefabByName( const char *name );

ApeEntity *YnCore_EntityManager_CreateEntity( void );
ApeEntity *YnCore_EntityManager_CreateEntityFromPrefab( const char *name );
void YnCore_EntityManager_DestroyEntity( ApeEntity *entity );

/**
 * Returns the total number of active entities.
 */
unsigned int YnCore_EntityManager_GetNumOfEntities( void );

bool ogeEntityManager_RegisterComponent( const char *name, const ApeEntityComponentCallbackTable *callbackTable );
const ApeEntityComponentBase *YnCore_EntityManager_GetComponentBaseByName( const char *name );
ApeEntityComponent *YnCore_EntityManager_AddComponentToEntity( ApeEntity *entity, const char *name );

/**
 * Returns a list of properties that can be modified for the component.
 */
const struct ApeEditorField *YnCore_EntityComponent_GetEditableProperties( const ApeEntityComponent *entityComponent, unsigned int *num );

/****************************************
 * ENTITY
 ****************************************/

NdBranch *YnCore_Entity_Serialize( ApeEntity *self, NdBranch *root );
ApeEntity *YnCore_Entity_Deserialize( NdBranch *root );

ApeEntityComponent *YnCore_Entity_GetComponentByName( ApeEntity *self, const char *name );
ApeEntityComponent *YnCore_Entity_AttachComponentByName( ApeEntity *self, const char *name );
void apeRemoveEntityComponent( ApeEntity *self, ApeEntityComponent *component );
void apeRemoveAllEntityComponents( ApeEntity *self );

PL_EXTERN_C_END
