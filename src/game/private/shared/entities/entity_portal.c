// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Deployable portal, mostly for testing but maybe eventually a mechanic?
// Author:  Mark E. Sowden

#include "../game_private.h"

static constexpr unsigned int MAX_PORTALS = 2;

static ApeEntity *portals[ MAX_PORTALS ];

typedef struct GamePortalEntity
{
	ApeBrush *brush;
} GamePortalEntity;
#define GAME_PORTAL_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), "portal", GamePortalEntity )

static ApeMaterial *portalMaterial;

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

	return PL_NEW( GamePortalEntity );
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
	assert( portal != nullptr );

	PL_DELETE( portal );
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

	static constexpr float TALL = 64.0f;
	static constexpr float WIDE = 32.0f;

	brush->numVertices   = 4;
	brush->vertices      = PL_NEW_( PLVector3, brush->numVertices );
	brush->vertices[ 0 ] = PL_VECTOR3( -WIDE, 0.0f, 0.0f );
	brush->vertices[ 1 ] = PL_VECTOR3( -WIDE, TALL, 0.0f );
	brush->vertices[ 2 ] = PL_VECTOR3( WIDE, TALL, 0.0f );
	brush->vertices[ 3 ] = PL_VECTOR3( WIDE, 0.0f, 0.0f );

	brush->numFaces = 1;
	brush->faces    = PL_NEW_( ApeBrushFace, brush->numFaces );

	ApeBrushFace *face = &brush->faces[ 0 ];
	face->parent       = brush;
	face->numVertices  = brush->numVertices;
	face->flags        = APE_BRUSH_FACE_FLAG_PORTAL;
	face->material     = portalMaterial;
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		ApeBrushFaceVertex *vertex = &face->vertices[ i ];
		vertex->position           = &brush->vertices[ i ];
		vertex->colour             = PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.0f );

		face->edgeLoop[ i ] = vertex;
	}

	ape_brush_face_compute_normal( face );

	ape_brush_compute_face_bounds( brush );
	ape_brush_compute_face_bounds( brush );

	portal->brush = brush;
}

static void tick_portal( ApeEntity *self, double delta )
{
	//TODO: cool animation of the portal magically closing and opening
}

ApeEntityClassDefinition game_portalEntityClass_ = {
        .name        = "portal",
        .description = "Teleporter which uses a procedural brush.",

        .createFunction  = create_portal,
        .destroyFunction = destroy_portal,
        .spawnFunction   = spawn_portal,
        .tickFunction    = tick_portal,
};
