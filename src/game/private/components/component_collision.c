// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "../game_private.h"

#include "component_collision.h"

static void *create_collision() { return QM_OS_MEMORY_NEW( GameCollisionComponent ); }
static void  destroy_collision( void *data )
{
	qm_os_memory_free( data );
}

static AcmBranch *serialize_collision( void *ptr, AcmBranch *root )
{
	GameCollisionComponent *self = ptr;
	acm_push_ui32( root, "groups", self->groups );
	return root;
}

static void *deserialize_collision( void *ptr, AcmBranch *root )
{
	GameCollisionComponent *self = ptr;
	self->groups                 = acm_get_uint( root, "groups", 0 );
	return self;
}

static ApePropertyEnum typesEnum[] = {
        {"None",     APE_COLLISION_TYPE_NONE    },
        {"AABB",     APE_COLLISION_TYPE_AABB    },
        {"Sphere",   APE_COLLISION_TYPE_SPHERE  },
        {"Cylinder", APE_COLLISION_TYPE_CYLINDER},
        {"Plane",    APE_COLLISION_TYPE_PLANE   },
};

static ApeProperty properties[] = {
        APE_PROPERTY_ENUM( "Type", "Type of collider.", GameCollisionComponent, type, typesEnum ),
};

ApeEntityComponentDefinition game_collisionComponent_ = {
        .name            = "collision",
        .createFunction  = create_collision,
        .destroyFunction = destroy_collision,

        .serializeFunction   = serialize_collision,
        .deserializeFunction = deserialize_collision,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),
};

/////////////////////////////////////////////////////////////////////////////////////
// Public API

void game_component_collision_debug_collider( const GameCollisionComponent *collision )
{
	switch ( collision->type )
	{
		case APE_COLLISION_TYPE_AABB:
			ape_draw_debug_aabb( &collision->collider.aabb, PL_COLOUR_BLUE );
			break;
		case APE_COLLISION_TYPE_CYLINDER:
			ape_draw_debug_cylinder( &collision->collider.cylinder, &PL_COLOUR_BLUE, 16 );
			break;
		case APE_COLLISION_TYPE_SPHERE:
			ape_draw_debug_sphere( collision->collider.sphere.origin,PL_COLOUR_BLUE,collision->collider.sphere.radius );
			break;
		default:
			break;
	}
}
