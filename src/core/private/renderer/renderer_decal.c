// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Decal and decal manager.
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "renderer.h"

//TODO: there should be a static list and a dynamic list here...

static constexpr unsigned int MAX_DECALS      = 4096;
static constexpr unsigned int MAX_DECAL_VERTS = 16;

static bool  showDebugDecals;
static float fadeThreshold;
static float decalOffset;
static int   decalMaxLife;

typedef struct ApeDecal
{
	unsigned int life;

	float angle;
	float scale;

	PLVector3 position;
	PLVector3 normal;
	PLVector3 tangent, bitangent;

	PLVector3    vertices[ MAX_DECAL_VERTS ];
	unsigned int numVertices;

	PLCollisionAABB bounds;

	ApeMaterial *material;

	ComSharedPtr *facePtr;
	ComSharedPtr *ptr;

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

static void cleanup_decal( ApeDecal *self )
{
	if ( self->material != nullptr )
	{
		ape_material_release( self->material );
		self->material = nullptr;
	}

	if ( self->node != nullptr )
	{
		PlDestroyLinkedListNode( self->node );
		self->node = nullptr;
	}

	if ( self->facePtr != nullptr )
	{
		com_shared_ptr_release( self->facePtr );
		self->facePtr = nullptr;
	}

	// decals are a little weird here (given they're from a pool),
	// so we need to set self to null first in the shared ptr
	com_shared_ptr_set( self->ptr, nullptr );
	com_shared_ptr_release( self->ptr );
	self->ptr = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////
// Decal Manager
// Decals are managed on a room-by-room basis.
/////////////////////////////////////////////////////////////////////////////////////

void ape_decal_manager_register_console_()
{
	PlRegisterConsoleVariable( "decal.showDebug", "Show debug spheres representing active decals.", "false", PL_VAR_BOOL, &showDebugDecals, nullptr, false );
	PlRegisterConsoleVariable( "decal.fadeThreshold", "", "0.2", PL_VAR_F32, &fadeThreshold, nullptr, true );
	PlRegisterConsoleVariable( "decal.offset", "Sets the offset from the wall.", "0.01", PL_VAR_F32, &decalOffset, nullptr, true );
	PlRegisterConsoleVariable( "decal.maxLife", "Sets the maximum lifetime of a decal.", "500", PL_VAR_I32, &decalMaxLife, nullptr, true );
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

		if ( decal->life >= decalMaxLife )
		{
			cleanup_decal( decal );
			continue;
		}

		if ( showDebugDecals )
		{
			ape_draw_debug_sphere( decal->position, PL_COLOUR_WHITE, decal->scale );
		}

		decal->life++;
	}
}

static bool decal_build_rect( ApeDecal *self )
{
	ApeBrushFace *face = com_shared_ptr_get( self->facePtr );
	if ( face == nullptr )
	{
		return false;
	}

	// don't ask me how anything below works (or why it's awful),
	// me is big dumb dumb when it comes to maths...

	com_math_plane_basis_vectors( &( ComMathPlane ) { .normal = face->normal }, &self->tangent, &self->bitangent );

	if ( self->angle != 0.0f )
	{
		PLMatrix4 rotation = PlRotateMatrix4( PL_DEG2RAD( self->angle ), &face->normal );
		self->tangent      = PlTransformVector3( &self->tangent, &rotation );
		self->bitangent    = PlTransformVector3( &self->bitangent, &rotation );
	}

	float     halfSize        = self->scale / 2.0f;
	PLVector3 tangentOffset   = PlScaleVector3F( self->tangent, halfSize );
	PLVector3 bitangentOffset = PlScaleVector3F( self->bitangent, halfSize );

	// first, setup the initial quad (and move slightly away from the face to avoid z-fighting)
	PLVector3 npos      = PlAddVector3( self->position, PlScaleVector3F( self->normal, decalOffset ) );
	self->numVertices   = 4;
	self->vertices[ 0 ] = PlAddVector3( npos, PlAddVector3( PlScaleVector3F( tangentOffset, 1.0f ), PlScaleVector3F( bitangentOffset, -1.0f ) ) );
	self->vertices[ 1 ] = PlAddVector3( npos, PlAddVector3( PlScaleVector3F( tangentOffset, 1.0f ), PlScaleVector3F( bitangentOffset, 1.0f ) ) );
	self->vertices[ 2 ] = PlAddVector3( npos, PlAddVector3( PlScaleVector3F( tangentOffset, -1.0f ), PlScaleVector3F( bitangentOffset, 1.0f ) ) );
	self->vertices[ 3 ] = PlAddVector3( npos, PlAddVector3( PlScaleVector3F( tangentOffset, -1.0f ), PlScaleVector3F( bitangentOffset, -1.0f ) ) );

	// and now, clip it
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		const PLVector3 a = *face->edgeLoop[ i ]->position;
		const PLVector3 b = *face->edgeLoop[ ( i + 1 ) % face->numVertices ]->position;

		PLVector3 edgeNormal = PlSubtractVector3( b, a );
		edgeNormal           = PlVector3CrossProduct( self->normal, edgeNormal );
		edgeNormal           = PlNormalizeVector3( edgeNormal );

		// setup the clip plane
		ComMathPlane plane = {};
		plane.normal       = edgeNormal;
		plane.distance     = -PlVector3DotProduct( edgeNormal, a );

		PLVector3 newClippedVertices[ MAX_DECAL_VERTS ] = { {} };

		self->numVertices = ape_renderer_clip_polygon( self->vertices, self->numVertices, &plane, newClippedVertices, MAX_DECAL_VERTS );
		if ( self->numVertices < 3 || self->numVertices >= MAX_DECAL_VERTS )
		{
			return false;
		}

		memcpy( self->vertices, newClippedVertices, self->numVertices * sizeof( PLVector3 ) );
	}

	return true;
}

ComSharedPtr *ape_decal_manager_create_decal_( ApeDecalManager *self, ApeBrushFace *face, ApeMaterial *material, const PLVector3 *pos, float angle, float scale )
{
	assert( face != nullptr );

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
		decal->angle    = angle;
		decal->scale    = scale;

		//TODO: add reference
		decal->material = material;

		if ( !decal_build_rect( decal ) )
		{
			com_shared_ptr_release( decal->facePtr );
			return nullptr;
		}

		decal->node = PlInsertLinkedListNode( self->decalList, decal );

		// because decals can be destroyed at runtime,
		// we'll need to use a shared ptr here...
		decal->ptr = com_shared_ptr_create( decal );

		return decal->ptr;
	}

	return nullptr;
}

ComSharedPtr *ape_decal_manager_create_projected_decal_( ApeDecalManager *self, ApeRoom *room, ApeMaterial *material, const PLVector3 *pos, const PLVector3 *dir, float angle, float scale )
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

#if 0
	// cheap shitty way to pass a decal to everything we hit
	//TODO: a single decal should be able to project onto multiple adjacent faces...
	//		we don't do this yet because it's not something we track :(

	PLCollisionSphere sphere = {};
	sphere.origin            = result.intersection;
	sphere.radius            = scale;

	ApeCollisionCollider collider = {};
	collider.type                 = APE_COLLISION_TYPE_SPHERE;
	collider.sphere               = &sphere;

	unsigned int              numHits;
	ApeCollisionIntersection *hits = ape_room_intersect( room, &collider, &numHits );
	if ( hits != nullptr )
	{
		ComSharedPtr *ptr = nullptr;
		for ( unsigned int i = 0; i < numHits; ++i )
		{
			if ( hits[ i ].face == nullptr )
			{
				continue;
			}

			ptr = ape_decal_manager_create_decal_( self, hits[ i ].face, material, &result.intersection, angle, scale );
			if ( ptr == nullptr )
			{
				return nullptr;
			}
		}

		PL_DELETE( hits );

		return ptr;
	}

	// uh, somehow we hit a ray against a surface but failed to find anything else???
	// alright then, just make a single decal at the point of intersection...
#endif

	ComSharedPtr *ptr = ape_decal_manager_create_decal_( self, result.face, material, &result.intersection, angle, scale );
	if ( ptr == nullptr )
	{
		return nullptr;
	}

	return ptr;
}

void ape_decal_manager_draw_( const ApeDecalManager *self )
{
	ApeDecal *decal;
	COM_ITERATE_LINKED_LIST( decal, self->decalList, i )
	{
		//TODO: optimise - batch - for now we'll just draw them like this for quickly getting them working

		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );

		float lifetime = ( float ) decal->life / ( float ) decalMaxLife;
		float fade     = 1.0f;
		if ( lifetime > fadeThreshold )
		{
			fade = 1.0f - ( lifetime - fadeThreshold ) / ( 1.0f - fadeThreshold );
		}

		float textureScale = 1.0f / decal->scale;
		for ( unsigned int j = 0; j < decal->numVertices; ++j )
		{
			PlgImmPushVertex( decal->vertices[ j ].x, decal->vertices[ j ].y, decal->vertices[ j ].z );
			PlgImmNormal( decal->normal.x, decal->normal.y, decal->normal.z );
			PlgImmColour( 255, 255, 255, PlFloatToByte( fade ) );

			// and now for the texture coords
			PLVector3 delta = PlSubtractVector3( decal->vertices[ j ], decal->position );
			PlgImmTextureCoord( 0.5f + PlVector3DotProduct( delta, decal->tangent ) * textureScale,
			                    0.5f + PlVector3DotProduct( delta, decal->bitangent ) * textureScale );
		}

		ape_material_draw( decal->material, mesh, nullptr );
	}
}
