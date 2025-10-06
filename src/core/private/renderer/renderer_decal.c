// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Decal and decal manager.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_shared_ptr.h"
#include "qmmath/public/qm_math_plane.h"

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

	QmMathVector3f position;
	QmMathVector3f normal;
	QmMathVector3f tangent, bitangent;

	QmMathVector3f vertices[ MAX_DECAL_VERTS ];
	unsigned int   numVertices;

	PLCollisionAABB bounds;

	ApeMaterial *material;

	QmOsSharedPtr *facePtr;
	QmOsSharedPtr *ptr;

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
		qm_os_shared_ptr_release( self->facePtr );
		self->facePtr = nullptr;
	}

	// decals are a little weird here (given they're from a pool),
	// so we need to set self to null first in the shared ptr
	qm_os_shared_ptr_set( self->ptr, nullptr );
	qm_os_shared_ptr_release( self->ptr );
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
	ApeDecalManager *manager = QM_OS_MEMORY_NEW( ApeDecalManager );

	manager->decalList = PlCreateLinkedList();
	if ( manager->decalList == nullptr )
	{
		ape_warning_( "Failed to create decals list: %s\n", PlGetError() );

		qm_os_memory_free( manager );
		manager = nullptr;

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
		ApeBrushFace *face = qm_os_shared_ptr_get( decal->facePtr );
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
	ApeBrushFace *face = qm_os_shared_ptr_get( self->facePtr );
	if ( face == nullptr )
	{
		return false;
	}

	// don't ask me how anything below works (or why it's awful),
	// me is big dumb dumb when it comes to maths...

	qm_math_plane_basis_vectors( &( QmMathPlane ) { .normal = face->normal }, &self->tangent, &self->bitangent );

	if ( self->angle != 0.0f )
	{
		PLMatrix4 rotation = PlRotateMatrix4( PL_DEG2RAD( self->angle ), &face->normal );
		self->tangent      = PlTransformVector3( &self->tangent, &rotation );
		self->bitangent    = PlTransformVector3( &self->bitangent, &rotation );
	}

	float          halfSize        = self->scale / 2.0f;
	QmMathVector3f tangentOffset   = qm_math_vector3f_scale_float( self->tangent, halfSize );
	QmMathVector3f bitangentOffset = qm_math_vector3f_scale_float( self->bitangent, halfSize );

	// first, setup the initial quad (and move slightly away from the face to avoid z-fighting)
	QmMathVector3f npos = qm_math_vector3f_add( self->position, qm_math_vector3f_scale_float( self->normal, decalOffset ) );
	self->numVertices   = 4;
	self->vertices[ 0 ] = qm_math_vector3f_add( npos, qm_math_vector3f_add( qm_math_vector3f_scale_float( tangentOffset, 1.0f ), qm_math_vector3f_scale_float( bitangentOffset, -1.0f ) ) );
	self->vertices[ 1 ] = qm_math_vector3f_add( npos, qm_math_vector3f_add( qm_math_vector3f_scale_float( tangentOffset, 1.0f ), qm_math_vector3f_scale_float( bitangentOffset, 1.0f ) ) );
	self->vertices[ 2 ] = qm_math_vector3f_add( npos, qm_math_vector3f_add( qm_math_vector3f_scale_float( tangentOffset, -1.0f ), qm_math_vector3f_scale_float( bitangentOffset, 1.0f ) ) );
	self->vertices[ 3 ] = qm_math_vector3f_add( npos, qm_math_vector3f_add( qm_math_vector3f_scale_float( tangentOffset, -1.0f ), qm_math_vector3f_scale_float( bitangentOffset, -1.0f ) ) );

	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	// and now, clip it
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		const QmMathVector3f a = brush->vertices[ face->vertices[ face->edgeLoopOrder[ i ] ].posIndex ];
		const QmMathVector3f b = brush->vertices[ face->vertices[ face->edgeLoopOrder[ ( i + 1 ) % face->numVertices ] ].posIndex ];

		QmMathVector3f edgeNormal = qm_math_vector3f_sub( b, a );
		edgeNormal                = qm_math_vector3f_cross_product( self->normal, edgeNormal );
		edgeNormal                = qm_math_vector3f_normalize( edgeNormal );

		// setup the clip plane
		QmMathPlane plane = {};
		plane.normal      = edgeNormal;
		plane.distance    = -qm_math_vector3f_dot_product( edgeNormal, a );

		QmMathVector3f newClippedVertices[ MAX_DECAL_VERTS ] = { {} };

		self->numVertices = ape_renderer_clip_polygon( self->vertices, self->numVertices, &plane, newClippedVertices, MAX_DECAL_VERTS );
		if ( self->numVertices < 3 || self->numVertices >= MAX_DECAL_VERTS )
		{
			return false;
		}

		memcpy( self->vertices, newClippedVertices, self->numVertices * sizeof( QmMathVector3f ) );
	}

	return true;
}

QmOsSharedPtr *ape_decal_manager_create_decal_( ApeDecalManager *self, ApeBrushFace *face, ApeMaterial *material, const QmMathVector3f *pos, float angle, float scale )
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
			face->ptr = qm_os_shared_ptr_create( face );
		}

		decal->facePtr = face->ptr;
		qm_os_shared_ptr_add( decal->facePtr );

		decal->position = *pos;
		decal->normal   = face->normal;
		decal->angle    = angle;
		decal->scale    = scale;

		//TODO: add reference
		decal->material = material;

		if ( !decal_build_rect( decal ) )
		{
			qm_os_shared_ptr_release( decal->facePtr );
			return nullptr;
		}

		decal->node = PlInsertLinkedListNode( self->decalList, decal );

		// because decals can be destroyed at runtime,
		// we'll need to use a shared ptr here...
		decal->ptr = qm_os_shared_ptr_create( decal );

		return decal->ptr;
	}

	return nullptr;
}

QmOsSharedPtr *ape_decal_manager_create_projected_decal_( ApeDecalManager *self, ApeRoom *room, ApeMaterial *material, const QmMathVector3f *pos, const QmMathVector3f *dir, float angle, float scale )
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
		QmOsSharedPtr *ptr = nullptr;
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

		qm_os_memory_free( hits );

		return ptr;
	}

	// uh, somehow we hit a ray against a surface but failed to find anything else???
	// alright then, just make a single decal at the point of intersection...
#endif

	QmOsSharedPtr *ptr = ape_decal_manager_create_decal_( self, result.face, material, &result.intersection, angle, scale );
	if ( ptr == nullptr )
	{
		return nullptr;
	}

	return ptr;
}

void ape_decal_manager_draw_( const ApeDecalManager *self )
{
	COM_PROFILE_FUNCTION_START();

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
			QmMathVector3f delta = qm_math_vector3f_sub( decal->vertices[ j ], decal->position );
			PlgImmTextureCoord( 0.5f + qm_math_vector3f_dot_product( delta, decal->tangent ) * textureScale,
			                    0.5f + qm_math_vector3f_dot_product( delta, decal->bitangent ) * textureScale );
		}

		ape_material_draw( decal->material, mesh, nullptr );
	}

	COM_PROFILE_FUNCTION_END();
}
