// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Trigger manager
// Author:  Mark E. Sowden

#include "../game_private.h"

#define TRIGGER_CLASS_NAME "trigger"

typedef struct TriggerEntity
{
	ApeWorldNode *body;
} TriggerEntity;
#define TRIGGER_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), TRIGGER_CLASS_NAME, TriggerEntity )

static void *create_trigger( ApeEntity *self, AcmBranch *properties )
{
	return QM_OS_MEMORY_NEW( TriggerEntity );
}

static void spawn_trigger( ApeEntity *self )
{
	TriggerEntity *trigger = TRIGGER_ENTITY( self );
	assert( trigger != nullptr );

	trigger->body = ape_world_node_get_child_by_name( APE_WORLD_NODE( self ), "body" );
	if ( trigger->body == nullptr )
	{
		game_warning_( "Trigger with no child body!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( self ) );
		return;
	}
}

static void destroy_trigger( ApeEntity *self )
{
	TriggerEntity *trigger = TRIGGER_ENTITY( self );
	assert( trigger != nullptr );
	qm_os_memory_free( trigger );
}

ApeEntityClassDefinition game_triggerEntityClass_ = {
        .name = TRIGGER_CLASS_NAME,

        .createFunction  = create_trigger,
        .spawnFunction   = spawn_trigger,
        .destroyFunction = destroy_trigger,
};
