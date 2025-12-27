// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Targets for the camera to follow.
// Author:  Mark E. Sowden

#include "game_private.h"

#include "entity_path.h"

static constexpr char ENTITY_PATH_CLASS_NAME[] = "qm1_camera_follow";

static PLHashTable *cameraFollowTable;

typedef struct PathEntity
{
	ApeStringProperty nextTarget[ APE_ENTITY_MAX_NAME ];
	ApeStringProperty viewTarget[ APE_ENTITY_MAX_NAME ];

	PLHashTableNode *followTableNode;
} PathEntity;
#define PATH_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), ENTITY_PATH_CLASS_NAME, PathEntity )

static void *create_entity_path( ApeEntity *self, AcmBranch *properties )
{
	if ( cameraFollowTable == nullptr )
	{
		cameraFollowTable = PlCreateHashTable();
	}

	return QM_OS_MEMORY_NEW( PathEntity );
}

static void destroy_entity_path( ApeEntity *self )
{
	PathEntity *followEntity = PATH_ENTITY( self );
	assert( followEntity != nullptr );

	PlDestroyHashTableNode( followEntity->followTableNode );
	if ( PlGetNumHashTableNodes( cameraFollowTable ) == 0 )
	{
		PlDestroyHashTable( cameraFollowTable );
		cameraFollowTable = nullptr;
	}

	qm_os_memory_free( followEntity );
}

ApeEntity *game_entity_path_get_first_()
{
	if ( cameraFollowTable == nullptr )
	{
		return nullptr;
	}

	PLHashTableNode *node = PlGetFirstHashTableNode( cameraFollowTable );
	if ( node == nullptr )
	{
		return nullptr;
	}

	ApeEntity *entity = PlGetHashTableNodeUserData( node );
	assert( entity != nullptr );

	return entity;
}

ApeEntity *game_entity_path_get_next_( ApeEntity *self )
{
	PathEntity *followEntity = PATH_ENTITY( self );
	assert( followEntity != nullptr );

	if ( *followEntity->nextTarget == '\0' )
	{
		return game_entity_path_get_first_();
	}

	ApeEntity *nextEntity = PlLookupHashTableUserData( cameraFollowTable, followEntity->nextTarget, strlen( followEntity->nextTarget ) );
	if ( nextEntity == nullptr )
	{
		return game_entity_path_get_first_();
	}

	return nextEntity;
}

static ApeProperty properties[] = {
        APE_PROPERTY_STRING( "Next Target", "Next camera follow target to move towards.", PathEntity, nextTarget ),
        APE_PROPERTY_STRING( "View Target", "What the camera should be aiming at.", PathEntity, viewTarget ),
};

ApeEntityClassDefinition game_pathEntityClass_ = {
        .name        = ENTITY_PATH_CLASS_NAME,
        .description = "Target for the camera to follow after level load.",

        .createFunction  = create_entity_path,
        .destroyFunction = destroy_entity_path,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),
};
