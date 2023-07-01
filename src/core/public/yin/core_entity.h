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

void apeInitializeEntityManager( void );
void apeShutdownEntityManager( void );
void apeTickEntityManager( void );
void ogeEntityManager_Draw( struct ApeCamera *camera, struct ApeWorldRoom *sector );
void YnCore_EntityManager_Save( NdBranch *root );
void ogeEntityManager_Restore( NdBranch *root );

// Prefabs
void apeRegisterEntityPrefab( const char *path );
void apeRegisterEntityPrefabs( void );
const ApeEntityPrefab *YnCore_EntityManager_GetPrefabByName( const char *name );

ApeEntity *apeCreateEntity( void );
ApeEntity *apeCreateEntityFromPrefab( const char *name );
void apeDestroyEntity( ApeEntity *entity );

/**
 * Returns the total number of active entities.
 */
unsigned int YnCore_EntityManager_GetNumOfEntities( void );

bool apeRegisterEntityComponent( const char *name, const ApeEntityComponentCallbackTable *callbackTable );
const ApeEntityComponentBase *apeGetEntityComponentBaseByName( const char *name );
ApeEntityComponent *apeAddEntityComponentToEntity( ApeEntity *entity, const char *name );

/**
 * Returns a list of properties that can be modified for the component.
 */
const struct ApeEditorField *apeGetEditableEntityComponentProperties( const ApeEntityComponent *entityComponent, unsigned int *num );

/****************************************
 * ENTITY
 ****************************************/

NdBranch *apeSerializeEntity( ApeEntity *self, NdBranch *root );
ApeEntity *apeDeserializeEntity( NdBranch *root );

ApeEntityComponent *apeGetEntityComponentByName( ApeEntity *self, const char *name );
ApeEntityComponent *apeAttachEntityComponentByName( ApeEntity *self, const char *name );
void apeRemoveEntityComponent( ApeEntity *self, ApeEntityComponent *component );
void apeRemoveAllEntityComponents( ApeEntity *self );

PL_EXTERN_C_END
