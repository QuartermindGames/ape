// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Deployable portal, mostly for testing but maybe eventually a mechanic?
// Author:  Mark E. Sowden

#include "../game_private.h"

static constexpr unsigned int MAX_PORTALS = 2;

static constexpr unsigned int NUM_VERTICES           = 4;
static constexpr float        TALL                   = 64.0f;
static constexpr float        WIDE                   = 16.0f;
static constexpr float        GAME_PORTAL_OPEN_SPEED = 4.0f;

static ApeEntity *portals[ MAX_PORTALS ];

static constexpr QmMathVector3f OPEN_VPOS[ NUM_VERTICES ] = {
        QM_MATH_VECTOR3F( -WIDE, 0.0f, 0.0f ),
        QM_MATH_VECTOR3F( -WIDE, TALL, 0.0f ),
        QM_MATH_VECTOR3F( WIDE, TALL, 0.0f ),
        QM_MATH_VECTOR3F( WIDE, 0.0f, 0.0f ),
};
static constexpr QmMathVector3f CLOSED_VPOS[ NUM_VERTICES ] = {
        QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ),
        QM_MATH_VECTOR3F( 0.0f, TALL, 0.0f ),
        QM_MATH_VECTOR3F( 0.0f, TALL, 0.0f ),
        QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ),
};

typedef enum GamePortalState
{
	GAME_PORTAL_STATE_OPENING,
	GAME_PORTAL_STATE_IDLE,
	GAME_PORTAL_STATE_CLOSING,
} GamePortalState;

typedef struct GamePortalEntity
{
	ApeBrush     *brush;
	ApeBrushFace *surface;

	QmMathVector3f startPos;
	QmMathVector3f startAng;

	GamePortalState state;

	unsigned int slot;
} GamePortalEntity;
#define GAME_PORTAL_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), "portal", GamePortalEntity )

static ApeMaterial *portalMaterial;

void game_entity_close_all_portals()
{
	for ( unsigned int i = 0; i < MAX_PORTALS; ++i )
	{
		if ( portals[ i ] == nullptr )
		{
			continue;
		}

		GamePortalEntity *portal = GAME_PORTAL_ENTITY( portals[ i ] );
		assert( portal != nullptr );

		portal->state = GAME_PORTAL_STATE_CLOSING;
	}
}

static void *create_portal( ApeEntity *self, AcmBranch *properties )
{
	unsigned int i;
	for ( i = 0; i < MAX_PORTALS; ++i )
	{
		if ( portals[ i ] != nullptr )
		{
			continue;
		}

		portals[ i ] = self;
		break;
	}

	if ( i >= MAX_PORTALS )
	{
		game_warning_( "Hit maximum entity portal limit (%u >= %u)!\n", i, MAX_PORTALS );
		return nullptr;
	}

	if ( portalMaterial == nullptr )
	{
		portalMaterial = ape_material_cache( "materials/world/test/portal.mat.n", APE_CACHE_GROUP_WORLD, true );
	}

	GamePortalEntity *portal = QM_OS_MEMORY_NEW( GamePortalEntity );
	portal->slot             = i;
	return portal;
}

static void destroy_portal( ApeEntity *self )
{
	for ( unsigned int i = 0; i < MAX_PORTALS; ++i )
	{
		if ( portals[ i ] != self )
		{
			continue;
		}

		portals[ i ] = nullptr;
		break;
	}

	GamePortalEntity *portal = GAME_PORTAL_ENTITY( self );
	qm_os_memory_free( portal );
}

static void spawn_portal( ApeEntity *self )
{
	GamePortalEntity *portal = GAME_PORTAL_ENTITY( self );
	assert( portal != nullptr );

	ApeBrush *brush = ape_brush_create( APE_WORLD_NODE( self ), "portal", &pl_vecOrigin3, &pl_vecOrigin3 );
	if ( brush == nullptr )
	{
		game_warning_( "Failed to attach brush for portal!\n" );
		return;
	}

	// build the face we're going to use for the brush here...
	// this is also a pretty good example of how you can
	// procedurally generate a brush at runtime

	portal->state = GAME_PORTAL_STATE_OPENING;

	brush->numVertices = NUM_VERTICES;
	brush->vertices    = QM_OS_MEMORY_NEW_( QmMathVector3f, brush->numVertices );
	for ( unsigned int i = 0; i < NUM_VERTICES; ++i )
	{
		brush->vertices[ i ] = CLOSED_VPOS[ i ];
	}

	brush->numFaces = 1;
	brush->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, brush->numFaces );

	ApeEntity        *lastPortalEntity = ( portal->slot > 0 ) ? portals[ portal->slot - 1 ] : nullptr;
	GamePortalEntity *lastPortal       = lastPortalEntity != nullptr ? GAME_PORTAL_ENTITY( lastPortalEntity ) : nullptr;

	ApeBrushFace *face = &brush->faces[ 0 ];
	face->parent       = brush;
	face->numVertices  = brush->numVertices;
	face->flags        = APE_BRUSH_FACE_FLAG_PORTAL;
	face->material     = portalMaterial;
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		ApeBrushFaceVertex *vertex = &face->vertices[ i ];
		vertex->posIndex           = i;
		vertex->colour             = QM_MATH_COLOUR4F( 1.0f, 1.0f, 1.0f, 1.0f );

		face->edgeLoopOrder[ i ] = i;
	}

	portal->surface = face;

	if ( lastPortal != nullptr )
	{
		face->destination = lastPortal->surface;
		if ( lastPortal->surface->destination == nullptr )
		{
			lastPortal->surface->destination = portal->surface;
		}
	}

	ape_brush_face_compute_normal( face );
	ape_brush_face_apply_material_coordinates( face, &QM_MATH_VECTOR2F( 1.0f, 1.0f ), &QM_MATH_VECTOR2F( 0.0f, 0.0f ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ), false );

	ape_brush_compute_face_bounds( brush );
	ape_brush_compute_bounds( brush );

	portal->startPos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	portal->startAng = ape_world_node_get_angles( APE_WORLD_NODE( self ) );

	portal->brush = brush;
}

static void tick_portal( ApeEntity *self, double delta )
{
	delta = game_get_delta_mod_( delta );

	GamePortalEntity *portal = GAME_PORTAL_ENTITY( self );
	assert( portal != nullptr );

	//TODO: cool animation of the portal magically closing and opening

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( self ) );

	unsigned int numTicks = ape_get_num_ticks();

	pos.y = portal->startPos.y + sinf( numTicks / 80.0f ) / 10.0f * 10.0f;

	ApeBrush *brush = portal->brush;
	switch ( portal->state )
	{
		default:
			break;
		case GAME_PORTAL_STATE_OPENING:
		{
			if ( portal->surface->destination == nullptr )
			{
				break;
			}

			bool updating = false;
			for ( unsigned int i = 0; i < NUM_VERTICES; ++i )
			{
				float d = qm_math_vector3f_length( qm_math_vector3f_sub( brush->vertices[ i ], OPEN_VPOS[ i ] ) );
				if ( d <= 1.0f )
				{
					continue;
				}

				brush->vertices[ i ] = PlLinearInterpolateV3f( brush->vertices[ i ], OPEN_VPOS[ i ], GAME_PORTAL_OPEN_SPEED * delta );

				updating = true;
			}

			ape_brush_compute_face_normals( brush );
			ape_brush_compute_face_bounds( brush );
			ape_brush_compute_bounds( brush );

			if ( !updating )
			{
				portal->state = GAME_PORTAL_STATE_IDLE;
			}
			break;
		}
		case GAME_PORTAL_STATE_IDLE:
		{
			break;
		}
		case GAME_PORTAL_STATE_CLOSING:
		{
			bool updating = false;
			for ( unsigned int i = 0; i < NUM_VERTICES; ++i )
			{
				float d = qm_math_vector3f_length( qm_math_vector3f_sub( brush->vertices[ i ], OPEN_VPOS[ i ] ) );
				if ( d >= 16.f )
				{
					continue;
				}

				brush->vertices[ i ] = PlLinearInterpolateV3f( brush->vertices[ i ], CLOSED_VPOS[ i ], GAME_PORTAL_OPEN_SPEED * 2.0f * delta );

				updating = true;
			}

			if ( !updating )
			{
				ape_world_node_destroy( APE_WORLD_NODE( self ) );
			}
			break;
		}
	}

	ape_world_node_set_position( APE_WORLD_NODE( self ), &pos );
	ape_world_node_set_angles( APE_WORLD_NODE( self ), &ang );
}

ApeEntityClassDefinition game_portalEntityClass_ = {
        .name        = "portal",
        .description = "Teleporter which uses a procedural brush.",

        .createFunction  = create_portal,
        .destroyFunction = destroy_portal,
        .spawnFunction   = spawn_portal,
        .tickFunction    = tick_portal,
};
