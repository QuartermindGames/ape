// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Creature, using procedural animation.
// Author:  Mark E. Sowden

#if defined( GAME_QM2 )

#	include "shared/game_private.h"
#	include "shared/ai/ai_brain.h"
#	include "shared/components/component_collision.h"
#	include "shared/components/component_health.h"
#	include "shared/components/component_movement.h"

static constexpr char QM2_CREATURE_CLASSNAME[] = "qm2_creature";

static constexpr unsigned int QM2_CREATURE_MAX_BONES  = 256;
static constexpr int16_t      QM2_CREATURE_MAX_HEALTH = 100;

typedef struct Qm2CreatureBone
{

} Qm2CreatureBone;

typedef enum Qm2CreatureRelationship
{
	QM2_CREATURE_RELATIONSHIP_GOOD,
	QM2_CREATURE_RELATIONSHIP_NEUTRAL,
	QM2_CREATURE_RELATIONSHIP_BAD,
} Qm2CreatureRelationship;

typedef enum Qm2CreatureSex
{
	QM2_CREATURE_SEX_INTERSEX,
	QM2_CREATURE_SEX_MALE,
	QM2_CREATURE_SEX_FEMALE,
} Qm2CreatureSex;

typedef enum Qm2CreatureStat
{
	QM2_CREATURE_STAT_AGE,
	QM2_CREATURE_STAT_HUNGER,
	QM2_CREATURE_STAT_THIRST,
	QM2_CREATURE_STAT_TEMPERATURE,
	QM2_CREATURE_STAT_STAMINA,

	QM2_CREATURE_MAX_STATS
} Qm2CreatureStat;

typedef struct Qm2CreatureEntity
{
	GameHealthComponent    *healthComponent;
	GameCollisionComponent *collisionComponent;

	Qm2CreatureBone bones[ QM2_CREATURE_MAX_BONES ];

	Qm2CreatureSex sex;
	int16_t        stats[ QM2_CREATURE_MAX_STATS ];

	AIBrain brain;
} Qm2CreatureEntity;

#	define QM2_CREATURE_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), QM2_CREATURE_CLASSNAME, Qm2CreatureEntity )

static void *create_creature( ApeEntity *self, AcmBranch *properties )
{
	PL_UNUSEDVAR( self );
	PL_UNUSEDVAR( properties );

	return PL_NEW( Qm2CreatureEntity );
}

static void destroy_creature( ApeEntity *self )
{
	Qm2CreatureEntity *creature = QM2_CREATURE_ENTITY( self );
	assert( creature != nullptr );

	PL_DELETE( creature );
}

static void spawn_creature( ApeEntity *self )
{
	Qm2CreatureEntity *creature = QM2_CREATURE_ENTITY( self );
	assert( creature != nullptr );

	if ( ( creature->healthComponent = ape_entity_add_component( self, "health" ) ) != nullptr )
	{
		creature->healthComponent->maxHealth = QM2_CREATURE_MAX_HEALTH;
		creature->healthComponent->health    = creature->healthComponent->maxHealth;
	}
}

static void tick_creature( ApeEntity *self, double delta )
{
	delta = game_get_delta_mod_( delta );
}

ApeEntityClassDefinition game_qm2_creatureEntityClass_ = {
        .name        = QM2_CREATURE_CLASSNAME,
        .description = "Our main attraction!",

        .createFunction  = create_creature,
        .destroyFunction = destroy_creature,
        .spawnFunction   = spawn_creature,
        .tickFunction    = tick_creature,
};

#endif
