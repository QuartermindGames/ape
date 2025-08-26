// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Creature, using procedural animation.
// Author:  Mark E. Sowden

#if defined( GAME_QM2 )

#	include "plcore/pl_array_vector.h"
#	include "qmos/public/qm_os_random.h"
#	include "shared/game_private.h"
#	include "shared/ai/ai_brain.h"
#	include "shared/components/component_collision.h"
#	include "shared/components/component_health.h"
#	include "shared/components/component_movement.h"

static constexpr int16_t QM2_CREATURE_MAX_HEALTH = 100;

static constexpr unsigned int QM2_CREATURE_MAX_SENSORS = 8;

static constexpr unsigned int QM2_CREATURE_MAX_BONES       = 256;
static constexpr unsigned int QM2_CREATURE_MAX_CHILD_BONES = 4;

typedef enum Qm2CreatureBoneType
{
	QM2_CREATURE_BONE_TYPE_DEFAULT,
	QM2_CREATURE_BONE_TYPE_NECK,
	QM2_CREATURE_BONE_TYPE_HEAD,
	QM2_CREATURE_BONE_TYPE_EYE,
	QM2_CREATURE_BONE_TYPE_LEG,
	QM2_CREATURE_BONE_TYPE_FOOT,
} Qm2CreatureBoneType;

typedef struct Qm2CreatureBone
{
	char name[ 64 ];

	unsigned int parent;

	Qm2CreatureBoneType type;

	struct Qm2CreatureBone *children[ QM2_CREATURE_MAX_CHILD_BONES ];
	unsigned int            numChildren;

	QmMathVector3f position;
} Qm2CreatureBone;

/////////////////////////////////////////////////////////////////////////////////////
// Creature Classes
/////////////////////////////////////////////////////////////////////////////////////

PLVectorArray *creatureClasses;

typedef struct Qm2CreatureClass
{
	char *name;

	ApeMaterial *icon;

	Qm2CreatureBone bones[ QM2_CREATURE_MAX_BONES ];
	unsigned int    numBones;
} Qm2CreatureClass;

static void destroy_creature_class( void *p )
{
	Qm2CreatureClass *creatureClass = p;

	if ( creatureClass->icon != nullptr )
	{
		ape_material_release( creatureClass->icon );
	}

	qm_os_memory_free( creatureClass->name );
	qm_os_memory_free( creatureClass );
}

static void parse_creature_bone( Qm2CreatureClass *creatureClass, AcmBranch *branch )
{
	Qm2CreatureBone *bone = &creatureClass->bones[ creatureClass->numBones ];

	snprintf( bone->name, sizeof( bone->name ), "%s", acm_get_string( branch, "name", "" ) );

	const char *typeName = acm_get_string( branch, "type", nullptr );
	if ( typeName != nullptr )
	{
		if ( strcmp( typeName, "neck" ) == 0 )
		{
			bone->type = QM2_CREATURE_BONE_TYPE_NECK;
		}
		else if ( strcmp( typeName, "head" ) == 0 )
		{
			bone->type = QM2_CREATURE_BONE_TYPE_HEAD;
		}
		else if ( strcmp( typeName, "eye" ) == 0 )
		{
			bone->type = QM2_CREATURE_BONE_TYPE_EYE;
		}
		else if ( strcmp( typeName, "leg" ) == 0 )
		{
			bone->type = QM2_CREATURE_BONE_TYPE_LEG;
		}
		else if ( strcmp( typeName, "foot" ) == 0 )
		{
			bone->type = QM2_CREATURE_BONE_TYPE_FOOT;
		}
		else
		{
			game_warning_( "Unknown bone type (%s) specified for creature class (%s)!\n", typeName, creatureClass->name );
		}
	}

	bone->position = com_acm_get_vector3( branch, "position", &pl_vecOrigin3 );

	bone->parent = acm_get_uint( branch, "parent", 0 );
	if ( bone->parent >= QM2_CREATURE_MAX_BONES )
	{
		game_warning_( "Invalid parent bone (%u) specified for creature class (%s)!\n", bone->parent, creatureClass->name );
	}

	creatureClass->numBones++;
}

static void cache_creature_class( const char *path, void *user )
{
	AcmBranch *root = com_acm_load_file( path, "creature" );
	if ( root == nullptr )
	{
		game_warning_( "Failed to load creature class (%s)!\n", path );
		return;
	}

	Qm2CreatureClass *creatureClass = QM_OS_MEMORY_NEW( Qm2CreatureClass );

	asprintf( &creatureClass->name, "%s", acm_get_string( root, "name", "unnamed" ) );

	creatureClass->icon = ape_material_cache( acm_get_string( root, "icon", "creatures/creature_fallback_icon.mat.n" ), APE_CACHE_GROUP_WORLD, true );

	AcmBranch *child;
	if ( ( child = acm_get_child_by_name( root, "bones" ) ) != nullptr )
	{
		ACM_ITERATE_BRANCH( child, i )
		{
			parse_creature_bone( creatureClass, i );
		}
	}

	PlPushBackVectorArrayElement( creatureClasses, creatureClass );

	acm_branch_destroy( root );
}

static void cache_creature_classes()
{
	creatureClasses = PlCreateVectorArray( 16 );
	if ( creatureClasses == nullptr )
	{
		game_error_( "Failed to create creature classes array: %s\n", PlGetError() );
		return;
	}

	PlScanDirectory( "creatures", "acm", cache_creature_class, false, nullptr );
}

/////////////////////////////////////////////////////////////////////////////////////
// Creature Entity
/////////////////////////////////////////////////////////////////////////////////////

static constexpr char QM2_CREATURE_CLASSNAME[] = "qm2_creature";

typedef enum Qm2CreatureSensorType
{
	QM2_CREATURE_SENSOR_TYPE_INVALID,
	QM2_CREATURE_SENSOR_TYPE_VISION,
	QM2_CREATURE_SENSOR_TYPE_SOUND,
} Qm2CreatureSensorType;

typedef struct Qm2CreatureSensor
{

} Qm2CreatureSensor;

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

	QM2_CREATURE_MAX_SEXES
} Qm2CreatureSex;

typedef enum Qm2CreatureAgePhase
{
	QM2_CREATURE_AGE_PHASE_BABY,
	QM2_CREATURE_AGE_PHASE_CHILD,
	QM2_CREATURE_AGE_PHASE_TEENAGER,
	QM2_CREATURE_AGE_PHASE_ADULT,
	QM2_CREATURE_AGE_PHASE_ELDERLY,
} Qm2CreatureAgePhase;

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
	unsigned int    numBones;

	Qm2CreatureSensor sensors[ QM2_CREATURE_MAX_SENSORS ];
	unsigned int      numSensors;

	Qm2CreatureSex sex;
	int16_t        stats[ QM2_CREATURE_MAX_STATS ];

	const Qm2CreatureClass *class;

	AIBrain brain;

	unsigned int seed;
} Qm2CreatureEntity;

#	define QM2_CREATURE_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), QM2_CREATURE_CLASSNAME, Qm2CreatureEntity )

static void *create_creature( ApeEntity *self, AcmBranch *properties )
{
	PL_UNUSEDVAR( self );
	PL_UNUSEDVAR( properties );

	return QM_OS_MEMORY_NEW( Qm2CreatureEntity );
}

static void destroy_creature( ApeEntity *self )
{
	Qm2CreatureEntity *creature = QM2_CREATURE_ENTITY( self );
	assert( creature != nullptr );

	qm_os_memory_free( creature );
}

static void spawn_creature( ApeEntity *self )
{
	Qm2CreatureEntity *creature = QM2_CREATURE_ENTITY( self );
	assert( creature != nullptr );

	creature->seed = qm_os_random_seed_initialize();

	//TEMP: we'll just use the first class for now
	creature->class = PlGetVectorArrayFront( creatureClasses );
	assert( creature->class != nullptr );

	if ( ( creature->healthComponent = ape_entity_add_component( self, "health" ) ) != nullptr )
	{
		creature->healthComponent->maxHealth = QM2_CREATURE_MAX_HEALTH;
		creature->healthComponent->health    = creature->healthComponent->maxHealth;
	}

	creature->sex = qm_os_random_int( &creature->seed ) % QM2_CREATURE_MAX_SEXES;

	memcpy( creature->bones, creature->class->bones, sizeof( Qm2CreatureBone ) * creature->class->numBones );
	creature->numBones = creature->class->numBones;
}

static void tick_creature( ApeEntity *self, double delta )
{
	delta = game_get_delta_mod_( delta );

	Qm2CreatureEntity *creature = QM2_CREATURE_ENTITY( self );
	assert( creature != nullptr );

	game_ai_brain_tick_( &creature->brain, delta );

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( self ) );

	ang.y += 5.0f * delta;
	ape_world_node_set_angles( APE_WORLD_NODE( self ), &ang );

	PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( self ) );

	for ( unsigned int i = 0; i < creature->numBones; ++i )
	{
		QmMathVector3f bonePos = PlTransformVector3( &creature->bones[ i ].position, &transform );
		ape_draw_debug_sphere( bonePos, PL_COLOUR_ORANGE, 2.0f );

		Qm2CreatureBone *parent        = &creature->bones[ creature->bones[ i ].parent ];
		QmMathVector3f   parentBonePos = PlTransformVector3( &parent->position, &transform );
		ape_draw_debug_arrow( bonePos, parentBonePos, PL_COLOUR_INDIAN_RED, 1.0f );
	}

	ape_draw_debug_sphere( pos, PL_COLOUR_WHITE, 1.0f );
}

ApeEntityClassDefinition game_qm2_creatureEntityClass_ = {
        .name        = QM2_CREATURE_CLASSNAME,
        .description = "Our main attraction!",

        .createFunction  = create_creature,
        .destroyFunction = destroy_creature,
        .spawnFunction   = spawn_creature,
        .tickFunction    = tick_creature,
        .cacheFunction   = cache_creature_classes,
};

#endif
