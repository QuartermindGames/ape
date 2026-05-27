// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Decal and decal manager.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_shared_ptr.h"
#include "qmmath/public/qm_math_plane.h"

#include "ape_private.h"
#include "core_console.h"
#include "renderer.h"

/**
 * TODO:
 *		- Cast decal on multiple faces
 *		- Batch decals
 *		- Use texture arrays
 *		- World should manage decal manager, not rooms...
 */

static constexpr unsigned int MAX_STATIC_DECALS = 1024;
static constexpr unsigned int MAX_TEMP_DECALS   = 1024;
static constexpr unsigned int MAX_DECALS        = MAX_STATIC_DECALS + MAX_TEMP_DECALS;
static constexpr unsigned int MAX_DECAL_VERTS   = 16;

static constexpr unsigned int INFINITE_LIFE = ( unsigned int ) -1;

static bool  showDebugDecals;
static float fadeThreshold;
static float decalOffset;
static int   decalMaxLife;

typedef struct ApeDecal
{
	ApeDecalManager *manager;

	unsigned int life;

	float angle;
	float scale;

	QmMathVector3f position;
	QmMathVector3f normal;
	QmMathVector3f tangent, bitangent;

	QmMathVector3f vertices[ MAX_DECAL_VERTS ];
	unsigned int   numVertices;

	ApeMaterial *material;

	QmOsSharedPtr *facePtr;
	QmOsSharedPtr *ptr;
} ApeDecal;

typedef struct ApeDecalManager
{
	ApeDecal     decals[ MAX_DECALS ];
	unsigned int tempPos, staticPos;

	unsigned int numTempDecals;
	unsigned int numStaticDecals;
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

	if ( self->facePtr != nullptr )
	{
		qm_os_shared_ptr_release( self->facePtr );
		self->facePtr = nullptr;
	}

	// decals are a little weird here (given they're from a pool),
	// so we need to set self to null first in the shared ptr
	if ( self->ptr != nullptr )
	{
		qm_os_shared_ptr_set( self->ptr, nullptr );
		qm_os_shared_ptr_release( self->ptr );
		self->ptr = nullptr;
	}

	assert( self->manager != nullptr );
	if ( self->life == INFINITE_LIFE )
	{
		assert( self->manager->numStaticDecals > 0 );
		self->manager->numStaticDecals--;
	}
	else
	{
		assert( self->manager->numTempDecals > 0 );
		self->manager->numTempDecals--;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Decal Manager
// Decals are managed on a room-by-room basis.
/////////////////////////////////////////////////////////////////////////////////////

void ape_decal_manager_register_console_()
{
	ape_console_var_register( "decal.showDebug", "Show debug spheres representing active decals.", "false", PL_VAR_BOOL, &showDebugDecals, nullptr, 0 );
	ape_console_var_register( "decal.fadeThreshold", "", "0.2", PL_VAR_F32, &fadeThreshold, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "decal.offset", "Sets the offset from the wall.", "0.01", PL_VAR_F32, &decalOffset, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "decal.maxLife", "Sets the maximum lifetime of a decal.", "500", PL_VAR_I32, &decalMaxLife, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
}

ApeDecalManager *ape_decal_manager_create_()
{
	return QM_OS_MEMORY_NEW( ApeDecalManager );
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
}

void ape_decal_manager_tick_( ApeDecalManager *self, double delta )
{
	COM_PROFILE_FUNCTION_START();

	for ( unsigned int i = 0; i < MAX_DECALS; ++i )
	{
		ApeDecal *decal = &self->decals[ i ];
		if ( decal->ptr == nullptr )
		{
			continue;
		}

		// check the decal is still attached to something
		ApeBrushFace *face = qm_os_shared_ptr_get( decal->facePtr );
		if ( face == nullptr )
		{
			cleanup_decal( decal );
			continue;
		}

		if ( decal->life != INFINITE_LIFE && decal->life >= decalMaxLife )
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

	COM_PROFILE_FUNCTION_END();
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
		PLMatrix4 rotation = PlRotateMatrix4( QM_MATH_DEG2RAD( self->angle ), &face->normal );
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

QmOsSharedPtr *ape_decal_manager_create_decal_( ApeDecalManager *self, ApeBrushFace *face, ApeMaterial *material, const QmMathVector3f *pos, float angle, float scale, bool isStatic )
{
	assert( face != nullptr );

	// I don't really know why in retrospect I decided to do this,
	// but we're using the same pool for both static and temp decals...
	unsigned int  maxDecals, basePos;
	unsigned int *iterator;
	if ( isStatic )
	{
		if ( self->numStaticDecals >= MAX_STATIC_DECALS )
		{
			ape_console_warning_( "Hit maximum static decal limit (%u >= %u)!\n", self->numStaticDecals, MAX_STATIC_DECALS );
			return nullptr;
		}

		maxDecals = MAX_STATIC_DECALS;
		iterator  = &self->staticPos;
		basePos   = 0;
	}
	else
	{
		if ( self->numTempDecals >= MAX_TEMP_DECALS )
		{
			ape_console_warning_( "Hit maximum temp decal limit (%u >= %u)!\n", self->numTempDecals, MAX_TEMP_DECALS );
			return nullptr;
		}

		maxDecals = MAX_TEMP_DECALS;
		iterator  = &self->tempPos;
		basePos   = MAX_STATIC_DECALS;
	}

	unsigned int startPos = *iterator;
	for ( unsigned int i = 0; i < maxDecals; ++i, ++*iterator )
	{
		// check if we've wrapped around
		if ( i > 0 && *iterator == startPos )
		{
			break;
		}

		if ( *iterator >= maxDecals )
		{
			*iterator = 0;
		}

		ApeDecal *decal = &self->decals[ basePos + *iterator ];
		if ( decal->ptr != nullptr )
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

		if ( isStatic )
		{
			decal->life = INFINITE_LIFE;

			self->numStaticDecals++;
		}
		else
		{
			self->numTempDecals++;
		}

		decal->manager = self;

		// because decals can be destroyed at runtime,
		// we'll need to use a shared ptr here...
		decal->ptr = qm_os_shared_ptr_create( decal );

		return decal->ptr;
	}

	return nullptr;
}

QmOsSharedPtr *ape_decal_manager_create_projected_decal_( ApeDecalManager *self, ApeRoom *room, ApeMaterial *material, const QmMathVector3f *pos, const QmMathVector3f *dir, float angle, float scale, bool isStatic )
{
	// we do the check a bit earlier here just to avoid the ray if we can,
	// this method should probably just be retired and we should leave the
	// caller to deal with the ray...
	if ( isStatic && self->numStaticDecals >= MAX_STATIC_DECALS )
	{
		ape_console_warning_( "Hit maximum static decal limit (%u >= %u)!\n", self->numStaticDecals, MAX_STATIC_DECALS );
		return nullptr;
	}
	if ( !isStatic && self->numTempDecals >= MAX_TEMP_DECALS )
	{
		ape_console_warning_( "Hit maximum temp decal limit (%u >= %u)!\n", self->numTempDecals, MAX_TEMP_DECALS );
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

			ptr = ape_decal_manager_create_decal_( self, hits[ i ].face, material, &result.intersection, angle, scale,TODO );
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

	QmOsSharedPtr *ptr = ape_decal_manager_create_decal_( self, result.face, material, &result.intersection, angle, scale, isStatic );
	if ( ptr == nullptr )
	{
		return nullptr;
	}

	return ptr;
}

void ape_decal_manager_draw_( const ApeDecalManager *self )
{
	COM_PROFILE_FUNCTION_START();

	for ( unsigned int i = 0; i < MAX_DECALS; ++i )
	{
		const ApeDecal *decal = &self->decals[ i ];
		if ( decal->ptr == nullptr )
		{
			continue;
		}

		//TODO: optimise - batch - for now we'll just draw them like this for quickly getting them working

		float fade = 1.0f;
		if ( decal->life != INFINITE_LIFE )
		{
			float lifetime = ( float ) decal->life / ( float ) decalMaxLife;
			if ( lifetime > fadeThreshold )
			{
				fade = 1.0f - ( lifetime - fadeThreshold ) / ( 1.0f - fadeThreshold );
			}

			// skip it if it's completely transparent
			if ( fade <= 0.0f )
			{
				continue;
			}
		}

		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );

		float textureScale = 1.0f / decal->scale;
		for ( unsigned int j = 0; j < decal->numVertices; ++j )
		{
			PlgImmPushVertex( decal->vertices[ j ].x, decal->vertices[ j ].y, decal->vertices[ j ].z );
			PlgImmNormal( decal->normal.x, decal->normal.y, decal->normal.z );
			PlgImmColour( 255, 255, 255, QM_MATH_FTOB( fade ) );

			// and now for the texture coords
			QmMathVector3f delta = qm_math_vector3f_sub( decal->vertices[ j ], decal->position );
			PlgImmTextureCoord( 0.5f + qm_math_vector3f_dot_product( delta, decal->tangent ) * textureScale,
			                    0.5f + qm_math_vector3f_dot_product( delta, decal->bitangent ) * textureScale );
		}

		ape_material_draw( decal->material, mesh, nullptr );
	}

	COM_PROFILE_FUNCTION_END();
}
