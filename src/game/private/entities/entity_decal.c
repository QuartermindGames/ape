// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Decal caster entity.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_shared_ptr.h"
#include "qmos/public/qm_os_string.h"

#include "game_private.h"

static constexpr char GAME_DECAL_ENTITY_CLASS_NAME[] = "decal";

static constexpr unsigned int MAX_DECAL_MATERIAL_NAME = 128;

typedef struct GameDecalEntity
{
	ApeStringProperty materialName[ MAX_DECAL_MATERIAL_NAME ];
	ApeFloatProperty  angle;
	ApeFloatProperty  scale;

	ApeMaterial *material;

	QmOsSharedPtr *decalPtr;
} GameDecalEntity;

#define GAME_DECAL_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_DECAL_ENTITY_CLASS_NAME, GameDecalEntity )

static void decal_entity_set_material( GameDecalEntity *decalEntity )
{
	assert( *decalEntity->materialName != '\0' );

	if ( decalEntity->material != nullptr )
	{
		ape_material_release_reference( decalEntity->material );
		decalEntity->material = nullptr;
	}

	char path[ 256 ];
	snprintf( path, sizeof( path ), "materials/decals/%s.mat.n", decalEntity->materialName );
	decalEntity->material = ape_material_cache( path, APE_CACHE_GROUP_WORLD, true );
}

static void  decal_entity_on_update_property( ApeEntity *self, [[maybe_unused]] const ApeProperty *property );
static void *decal_entity_create( ApeEntity *self )
{
	GameDecalEntity *decalEntity = QM_OS_MEMORY_NEW( GameDecalEntity );
	if ( decalEntity != nullptr )
	{
		decalEntity->scale = 100.0f;

		static const char *defaultMaterial = "decal_sheet_default";
		snprintf( decalEntity->materialName, sizeof( decalEntity->materialName ), "%s", defaultMaterial );

		decal_entity_set_material( decalEntity );
	}

	return decalEntity;
}

static void decal_entity_destroy( ApeEntity *self )
{
	GameDecalEntity *decalEntity = GAME_DECAL_ENTITY( self );
	assert( decalEntity != nullptr );

	// uuurrggghh so we don't want to destroy the
	// decal in editor mode, but we do want to destroy
	// it after spawn, so here's a dumb check
	ApeEntityState state = ape_entity_get_state( self );
	if ( decalEntity->decalPtr != nullptr && !( state == APE_ENTITY_STATE_SPAWNED || state == APE_ENTITY_STATE_SPAWNING ) )
	{
		ApeDecal *decal = qm_os_shared_ptr_get( decalEntity->decalPtr );
		if ( decal != nullptr )
		{
			ape_decal_free( decal );
		}

		qm_os_shared_ptr_release( decalEntity->decalPtr );
	}

	if ( decalEntity->material != nullptr )
	{
		ape_material_release_reference( decalEntity->material );
		decalEntity->material = nullptr;
	}

	qm_os_memory_free( decalEntity );
}

static QmMathVector3f decal_entity_get_projection_dir( ApeEntity *self )
{
	QmMathVector3f dir;
	PlAnglesAxes(
	        ape_world_node_get_angles( APE_WORLD_NODE( self ) ),
	        nullptr, nullptr, &dir );
	return qm_math_vector3f_normalize( dir );
}

static QmOsSharedPtr *decal_entity_trace_decal( ApeEntity *self )
{
	GameDecalEntity *decalEntity = GAME_DECAL_ENTITY( self );
	assert( decalEntity != nullptr );

	//TODO: we don't *really* need to trace a decal for this every time...
	//		instead rework this eventually to just trace a sphere

	PLCollisionRay ray = {};
	ray.origin         = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	ray.direction      = decal_entity_get_projection_dir( self );

	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( self ) );
	assert( room != nullptr );

	ApeCollisionIntersection result = {};
	if ( ape_room_ray_intersect( room, &ray, &result ) && result.face != nullptr )
	{
		return ape_room_create_decal( room,
		                              decalEntity->material,
		                              result.face,
		                              result.intersection,
		                              decalEntity->angle,
		                              decalEntity->scale,
		                              true );
	}

	return nullptr;
}

static void decal_entity_spawn( ApeEntity *self )
{
	GameDecalEntity *decalEntity = GAME_DECAL_ENTITY( self );
	assert( decalEntity != nullptr );

	decal_entity_set_material( decalEntity );

	decalEntity->decalPtr = decal_entity_trace_decal( self );
	qm_os_shared_ptr_add( decalEntity->decalPtr );

	// after it's projected, we don't need the entity any more
	//TODO: well, unless this is something we want to trigger later,
	//		but we don't have a mechanism for that yet :)
	ape_world_node_destroy( APE_WORLD_NODE( self ) );
}

static void decal_entity_on_update_property( ApeEntity *self, [[maybe_unused]] const ApeProperty *property )
{
	GameDecalEntity *decalEntity = GAME_DECAL_ENTITY( self );
	assert( decalEntity != nullptr );

	decal_entity_set_material( decalEntity );

	// clean up the existing decal
	//TODO: this is gross, this should update the existing decal, not keep recreating it!!
	if ( decalEntity->decalPtr != nullptr )
	{
		ApeDecal *decal = qm_os_shared_ptr_get( decalEntity->decalPtr );
		if ( decal != nullptr )
		{
			ape_decal_free( decal );
		}

		qm_os_shared_ptr_release( decalEntity->decalPtr );
	}

	decalEntity->decalPtr = decal_entity_trace_decal( self );
	qm_os_shared_ptr_add( decalEntity->decalPtr );
}

static void decal_entity_on_draw_editor( ApeEntity *self, const bool isSelected )
{
	GameDecalEntity *decalEntity = GAME_DECAL_ENTITY( self );
	assert( decalEntity != nullptr );

	if ( isSelected )
	{
		QmMathColour4ub colour = decalEntity->decalPtr != nullptr ? PL_COLOUR_GREEN : PL_COLOUR_RED;

		QmMathVector3f dir = decal_entity_get_projection_dir( self );

		QmMathVector3f startPos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
		QmMathVector3f endPos   = qm_math_vector3f_scale( startPos, qm_math_vector3f_scale_float( dir, 10.0f ) );
		ape_draw_debug_arrow( startPos, endPos, colour, 2.0f );

		// urgh, for all the time we're selected, just keep reprojecting
		decal_entity_on_update_property( self, nullptr );
		assert( decalEntity->material != nullptr );
	}
}

static ApeProperty properties[] = {
        APE_PROPERTY_STRING( "Material Name", "Name of the material to use (relative to 'materials/decals/<materialName>.mat.n'.", GameDecalEntity, materialName ),
        APE_PROPERTY_BASIC( "Angle", "Angle of the decal.", GameDecalEntity, angle, FLOAT ),
        APE_PROPERTY_BASIC( "Scale", "Scale of the decal.", GameDecalEntity, scale, FLOAT ),
};

ApeEntityClassDefinition game_decalEntityClass_ = {
        .name        = GAME_DECAL_ENTITY_CLASS_NAME,
        .description = "Casts a decal in the specified direction. Is destroyed after spawn.",

        .createFunction  = decal_entity_create,
        .destroyFunction = decal_entity_destroy,
        .spawnFunction   = decal_entity_spawn,

        .onUpdateProperty = decal_entity_on_update_property,
        .onDrawEditor     = decal_entity_on_draw_editor,

        .properties       = properties,
        .numProperties    = QM_OS_ARRAY_ELEMENTS( properties ),
        .editorSpritePath = "materials/editor/icons/icon_decal.mat.n",
};
