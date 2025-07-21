// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Decal and decal manager.
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "renderer.h"

static constexpr unsigned int MAX_DECALS = 4096;

static bool  showDebugDecals;
static float fadeThreshold;
static float decalOffset;

typedef struct ApeDecal
{
	unsigned int maxLife;
	unsigned int life;

	float size;

	PLVector3 position;
	PLVector3 normal;

	PLVector3 rect[ 4 ];

	PLCollisionAABB bounds;

	ApeMaterial *material;

	ComSharedPtr *facePtr;

	PLLinkedListNode *node;
} ApeDecal;

typedef struct ApeDecalManager
{
	PLLinkedList *decalList;
	ApeDecal      decals[ MAX_DECALS ];
	unsigned int  iteratorPos;
} ApeDecalManager;

/////////////////////////////////////////////////////////////////////////////////////
// Decal
/////////////////////////////////////////////////////////////////////////////////////

static void cleanup_decal( ApeDecal *decal )
{
	if ( decal->material != nullptr )
	{
		ape_material_release( decal->material );
		decal->material = nullptr;
	}

	if ( decal->node != nullptr )
	{
		PlDestroyLinkedListNode( decal->node );
		decal->node = nullptr;
	}

	if ( decal->facePtr != nullptr )
	{
		com_shared_ptr_release( decal->facePtr );
		decal->facePtr = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Decal Manager
// Decals are managed on a room-by-room basis.
/////////////////////////////////////////////////////////////////////////////////////

void ape_decal_manager_register_console_()
{
	PlRegisterConsoleVariable( "decal.showDebug", "Show debug spheres representing active decals.", "false", PL_VAR_BOOL, &showDebugDecals, nullptr, false );
	PlRegisterConsoleVariable( "decal.fadeThreshold", "", "0.75", PL_VAR_F32, &fadeThreshold, nullptr, true );
	PlRegisterConsoleVariable( "decal.offset", "Sets the offset from the wall.", "0.01", PL_VAR_F32, &decalOffset, nullptr, true );
}

ApeDecalManager *ape_decal_manager_create_()
{
	ApeDecalManager *manager = PL_NEW( ApeDecalManager );

	manager->decalList = PlCreateLinkedList();
	if ( manager->decalList == nullptr )
	{
		ape_warning_( "Failed to create decals list: %s\n", PlGetError() );
		PL_DELETEN( manager );
		return nullptr;
	}

	return manager;
}

void ape_decal_manager_destroy_( ApeDecalManager *self )
{
	ape_decal_manager_clear_( self );
}

void ape_decal_manager_deserialize_( ApeDecalManager *self, AcmBranch *root )
{
#if 0//TODO: how can we link a decal to a face here... ?
	AcmBranch *branch = acm_get_child_by_name( root, "decals" );
	if ( branch == nullptr )
	{
		return;
	}
#endif
}

void ape_decal_manager_serialize_( ApeDecalManager *self, AcmBranch *root )
{
#if 0//TODO: how can we link a decal to a face here... ?
	unsigned int numDecals = PlGetNumLinkedListNodes( self->decalList );
	if ( numDecals == 0 )
	{
		return;
	}

	AcmBranch *branch = acm_push_array_object( root, "decals" );

	ApeDecal *decal;
	COM_ITERATE_LINKED_LIST( decal, self->decalList, i )
	{

	}
#endif
}

void ape_decal_manager_clear_( ApeDecalManager *self )
{
	for ( unsigned int i = 0; i < MAX_DECALS; ++i )
	{
		cleanup_decal( &self->decals[ i ] );
	}

	PlDestroyLinkedListNodes( self->decalList );
}

void ape_decal_manager_tick_( ApeDecalManager *self, double delta )
{
	ApeDecal *decal;
	COM_ITERATE_LINKED_LIST( decal, self->decalList, i )
	{
		// check the decal is still attached to something
		ApeBrushFace *face = com_shared_ptr_get( decal->facePtr );
		if ( face == nullptr )
		{
			cleanup_decal( decal );
			continue;
		}

		if ( decal->life >= decal->maxLife )
		{
			cleanup_decal( decal );
			continue;
		}

		float lifetime = 1.0f - ( float ) decal->life / ( float ) decal->maxLife;
		float fade     = 1.0f;
		if ( fade > fadeThreshold )
		{
			fade = ( lifetime - fadeThreshold ) / fadeThreshold;
		}

		PLColourF32 colour = PL_COLOURF32( 1.0f, 1.0f, 1.0f, fade );

		if ( showDebugDecals )
		{
			ape_draw_debug_sphere( decal->position, PL_COLOURF32_TO_U8( colour ), decal->size );
		}

		decal->life++;
	}
}

static void decal_build_rect( ApeDecal *self )
{
}

ApeDecal *ape_decal_manager_create_decal_( ApeDecalManager *self, ApeBrushFace *face, ApeMaterial *material, const PLVector3 *pos )
{
	unsigned int numDecals = PlGetNumLinkedListNodes( self->decalList );
	if ( numDecals >= MAX_DECALS )
	{
		ape_warning_( "Failed to create decal, hit decal limit (%u >= %u)!\n", numDecals, MAX_DECALS );
		return nullptr;
	}

	for ( unsigned int i = 0; i < MAX_DECALS; ++i, ++self->iteratorPos )
	{
		if ( self->iteratorPos >= MAX_DECALS )
		{
			self->iteratorPos = 0;
		}

		ApeDecal *decal = &self->decals[ self->iteratorPos ];
		if ( decal->node != nullptr )
		{
			continue;
		}

		*decal = ( ApeDecal ) {};

		decal->node = PlInsertLinkedListNode( self->decalList, decal );

		if ( face->ptr == nullptr )
		{
			// Urgh, this is a botch to work
			// around the fact that brushes are
			// created procedurally...
			face->ptr = com_shared_ptr_create( face );
		}

		decal->facePtr = face->ptr;
		com_shared_ptr_add( decal->facePtr );

		decal->position = *pos;
		decal->normal   = face->normal;

		decal->size = 2.0f;

		decal->maxLife = 500;

		//TODO: add reference
		decal->material = material;

		decal_build_rect( decal );

		return decal;
	}

	return nullptr;
}

ApeDecal *ape_decal_manager_create_projected_decal_( ApeDecalManager *self, ApeRoom *room, ApeMaterial *material, const PLVector3 *pos, const PLVector3 *dir )
{
	unsigned int numDecals = PlGetNumLinkedListNodes( self->decalList );
	if ( numDecals >= MAX_DECALS )
	{
		return nullptr;
	}

	PLCollisionRay ray = {};
	ray.origin         = *pos;
	ray.direction      = *dir;

	ApeCollisionIntersection result = {};
	if ( !ape_room_ray_intersect( room, &ray, &result ) || result.face == nullptr )
	{
		return nullptr;
	}

	ApeDecal *decal = ape_decal_manager_create_decal_( self, result.face, material, &result.intersection );
	if ( decal == nullptr )
	{
		return nullptr;
	}

	return decal;
}

void ape_decal_manager_draw_( const ApeDecalManager *self )
{
	ApeDecal *decal;
	COM_ITERATE_LINKED_LIST( decal, self->decalList, i )
	{
		//TODO: optimise - batch - for now we'll just draw them like this for quickly getting them working

		ApeBrushFace *face = com_shared_ptr_get( decal->facePtr );
		if ( face == nullptr )
		{
			continue;
		}

		PLVector3 tangent, bitangent;
		com_math_plane_basis_vectors( &( ComMathPlane ) { .normal = face->normal }, &tangent, &bitangent );

		float     halfSize        = decal->size / 2.0f;
		PLVector3 tangentOffset   = PlScaleVector3F( tangent, halfSize );
		PLVector3 bitangentOffset = PlScaleVector3F( bitangent, halfSize );

		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

		static constexpr float UVS[ 4 ][ 2 ] = {
		        {0.0f, 1.0f}, // bottom-left
		        {1.0f, 1.0f}, // bottom-right
		        {0.0f, 0.0f}, // top-left
		        {1.0f, 0.0f}  // top-right
		};

		static constexpr float SIGNS[ 4 ][ 2 ] = {
		        {-1.0f, -1.0f}, // bottom-left
		        {1.0f,  -1.0f}, // bottom-right
		        {-1.0f, 1.0f }, // top-left
		        {1.0f,  1.0f }  // top-right
		};

		for ( unsigned int j = 0; j < 4; ++j )
		{
			// need to project away from the surface ever so slightly...
			PLVector3 npos   = PlAddVector3( decal->position, PlScaleVector3F( decal->normal, decalOffset ) );
			PLVector3 vertex = PlAddVector3( npos,
			                                 PlAddVector3(
			                                         PlScaleVector3F( tangentOffset, SIGNS[ j ][ 0 ] ),
			                                         PlScaleVector3F( bitangentOffset, SIGNS[ j ][ 1 ] ) ) );

			PlgImmPushVertex( vertex.x, vertex.y, vertex.z );
			PlgImmTextureCoord( UVS[ j ][ 0 ], UVS[ j ][ 1 ] );
		}

		ape_material_draw( decal->material, mesh, nullptr );
	}
}
