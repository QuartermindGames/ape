// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_linkedlist.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

typedef char YNCoreEntityClassName[ 64 ];
typedef char YNCoreEntityName[ 64 ];

typedef struct YNCoreEntity YNCoreEntity;
typedef struct YNCoreEntityPrefab YNCoreEntityPrefab;
typedef struct YNCoreEntityComponentBase YNCoreEntityComponentBase;
typedef struct YNCoreEntityComponent
{
	const YNCoreEntityComponentBase *base;
	YNCoreEntity *entity;
	struct PLLinkedListNode *listNode;
	void *userData;
} YNCoreEntityComponent;

#define ENTITY_COMPONENT_CAST( SELF, TYPE ) ( ( TYPE * ) ( SELF )->userData )

typedef void ( *YNCoreECSpawnFunction )( YNCoreEntityComponent *self );
typedef void ( *YNCoreECTickFunction )( YNCoreEntityComponent *self );
typedef void ( *YNCoreECDrawFunction )( YNCoreEntityComponent *self );
typedef void ( *YNCoreECDestroyFunction )( YNCoreEntityComponent *self );
typedef NdBranch *( *YNCoreECSerializeFunction )( YNCoreEntityComponent *self, NdBranch *root );
typedef NdBranch *( *YNCoreECDeserializeFunction )( YNCoreEntityComponent *self, NdBranch *root );

typedef struct YNCoreEntityComponentCallbackTable
{
	YNCoreECSpawnFunction spawnFunction;
	YNCoreECTickFunction tickFunction;
	YNCoreECDrawFunction drawFunction;
	YNCoreECDestroyFunction destroyFunction;
	YNCoreECSerializeFunction serializeFunction;
	YNCoreECDeserializeFunction deserializeFunction;

	const struct YNCoreEditorField *editorFields;
	unsigned int numEditorFields;
} YNCoreEntityComponentCallbackTable;

void YnCore_EntityManager_Initialize( void );
void YnCore_EntityManager_Shutdown( void );
void YnCore_EntityManager_Tick( void );
void YnCore_EntityManager_Draw( OgeCamera *camera, OgeWorldSector *sector );
void YnCore_EntityManager_Save( NdBranch *root );
void YnCore_EntityManager_Restore( NdBranch *root );

// Prefabs
void YnCore_EntityManager_RegisterEntityPrefab( const char *path );
void YnCore_EntityManager_RegisterEntityPrefabs( void );
const YNCoreEntityPrefab *YnCore_EntityManager_GetPrefabByName( const char *name );

YNCoreEntity *YnCore_EntityManager_CreateEntity( void );
YNCoreEntity *YnCore_EntityManager_CreateEntityFromPrefab( const char *name );
void YnCore_EntityManager_DestroyEntity( YNCoreEntity *entity );

/**
 * Returns the total number of active entities.
 */
unsigned int YnCore_EntityManager_GetNumOfEntities( void );

bool YnCore_EntityManager_RegisterComponent( const char *name, const YNCoreEntityComponentCallbackTable *callbackTable );
const YNCoreEntityComponentBase *YnCore_EntityManager_GetComponentBaseByName( const char *name );
YNCoreEntityComponent *YnCore_EntityManager_AddComponentToEntity( YNCoreEntity *entity, const char *name );

/**
 * Returns a list of properties that can be modified for the component.
 */
const struct YNCoreEditorField *YnCore_EntityComponent_GetEditableProperties( const YNCoreEntityComponent *entityComponent, unsigned int *num );

/****************************************
 * ENTITY
 ****************************************/

NdBranch *YnCore_Entity_Serialize( YNCoreEntity *self, NdBranch *root );
YNCoreEntity *YnCore_Entity_Deserialize( NdBranch *root );

YNCoreEntityComponent *YnCore_Entity_GetComponentByName( YNCoreEntity *self, const char *name );
YNCoreEntityComponent *YnCore_Entity_AttachComponentByName( YNCoreEntity *self, const char *name );
void YnCore_Entity_RemoveComponent( YNCoreEntity *self, YNCoreEntityComponent *component );
void YnCore_Entity_RemoveAllComponents( YNCoreEntity *self );

PL_EXTERN_C_END
