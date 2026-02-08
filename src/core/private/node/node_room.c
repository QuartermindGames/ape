// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include <plcore/pl_hashtable.h>

#include "qmos/public/qm_os_string.h"

#include "world/world.h"
#include "ape/ape_public_game.h"

#include "renderer/renderer.h"
#include "renderer/renderer_texture.h"

#include "yin/core_game.h"

static void *create_room( ApeWorldNode *parent )
{
	ApeRoom *room = QM_OS_MEMORY_NEW( ApeRoom );
	ape_world_node_setup_( &room->base, parent, APE_WORLD_NODE_TYPE_ROOM, nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );

	room->gravity = qm_math_vector3f( 0.0f, -0.9f, 0.0f );

	room->taggedSurfaceLookup = PlCreateHashTable();

	room->decalManager = ape_decal_manager_create_();

	return room;
}

ApeRoom *ape_room_create( ApeWorldNode *parent, const char *name )
{
	ApeRoom *room = create_room( parent );
	ape_world_node_set_name( APE_WORLD_NODE( room ), name );
	return room;
}

static void destroy_room( void *data, ApeWorldNode *parent )
{
	ApeRoom *self = data;

	// notify the game that a room is being destroy
	if ( ape_gameInterface->onDestroyRoom != nullptr )
	{
		ape_gameInterface->onDestroyRoom( self );
	}

	PlDestroyHashTable( self->taggedSurfaceLookup );

	ape_decal_manager_destroy_( self->decalManager );

	qm_os_memory_free( self->lightmap );
	if ( self->lightmapTexture != nullptr )
	{
		ape_texture_release_( self->lightmapTexture );
	}

	qm_os_memory_free( self );
}

void ape_room_set_ambience( ApeRoom *self, QmMathColour4f ambience )
{
	self->ambientLight = ambience;
}

QmMathColour4f ape_room_get_ambience( const ApeRoom *self )
{
	return self->ambientLight;
}

void ape_room_set_reverb_preset( ApeRoom *self, ApeAudioReverbPreset reverbPreset )
{
	self->reverbPreset = reverbPreset;
}

ApeAudioReverbPreset ape_room_get_reverb_preset( const ApeRoom *self )
{
	return self->reverbPreset;
}

void ape_room_add_tagged_surface( ApeRoom *self, ApeBrushFace *face )
{
	if ( PlInsertHashTableNode( self->taggedSurfaceLookup, face->tag, strlen( face->tag ), face ) == nullptr )
	{
		ape_console_warning_( "Attempted to add duplicate surface tag (%s)!\n", face->tag );
		return;
	}

	ape_console_verbose_( "Added \"%s\" to room lookup\n", face->tag );
}

void ape_room_remove_tagged_surface( ApeRoom *self, ApeBrushFace *face )
{
	PLHashTableNode *node = PlLookupHashTableNode( self->taggedSurfaceLookup, face->tag, strlen( face->tag ) );
	if ( node == nullptr )
	{
		ape_console_warning_( "Failed to remove tag (%s), lookup failed!\n", face->tag );
		return;
	}

	PlDestroyHashTableNode( node );

	ape_console_verbose_( "Removed \"%s\" from room lookup\n", face->tag );
}

ApeBrushFace *ape_room_get_tagged_surface( const ApeRoom *self, const char *tag )
{
	return PlLookupHashTableUserData( self->taggedSurfaceLookup, tag, strlen( tag ) );
}

const char *ape_room_set_unique_surface_tag( const ApeRoom *self, ApeBrushFace *face )
{
	unsigned int numTags = PlGetNumHashTableNodes( self->taggedSurfaceLookup );
	for ( ;; numTags++ )
	{
		char tag[ APE_BRUSH_FACE_MAX_TAG ];
		snprintf( tag, sizeof( tag ), "entry_%u", numTags );

		ApeBrushFace *taggedFace = ape_room_get_tagged_surface( self, tag );
		if ( taggedFace == nullptr )
		{
			ape_brush_face_set_tag( face, tag );
			break;
		}
	}

	return face->tag;
}

#if 0
/**
 * A very gross method to form a path for a lightmap, from the given room.
 */
static char *get_lightmap_path( ApeRoom *self )
{
	ApeWorldNode *root = ape_world_node_get_root( APE_WORLD_NODE( self ) );
	if ( root == nullptr )
	{
		ape_console_warning_( "Failed to get get room root!\n" );
		return nullptr;
	}

	const char *path = ape_world_node_get_path( root );
	if ( path == nullptr )
	{
		ape_console_warning_( "Failed to get root path!\n" );
		return nullptr;
	}

	char *buf = qm_os_string_alloc( "%s", path );
	if ( buf == nullptr )
	{
		ape_console_warning_( "Failed to allocate string!\n" );
		return nullptr;
	}

	static constexpr char LIGHTMAP_EXTENSION[] = ".lmp";

	size_t s = strlen( buf );
	if ( pl_strcasecmp( &buf[ s - strlen( APE_WORLD_ROOM_EXTENSION ) - 1 ], "." APE_WORLD_ROOM_EXTENSION ) == 0 )
	{
		strcpy( &buf[ s - 6 ], LIGHTMAP_EXTENSION );
	}
	else
	{
		char *c = strrchr( buf, '.' );
		if ( c == nullptr )
		{
			ape_console_warning_( "Failed to fetch file extension (%s)!\n", buf );
			goto cleanup;
		}

		*c = '\0';

		char *nbuf = qm_os_string_alloc( "%s.lmp", buf );
		if ( nbuf == nullptr )
		{
			ape_console_warning_( "Failed to allocate string!\n" );
			goto cleanup;
		}

		qm_os_memory_free( buf );
		buf = nbuf;
	}

	return buf;

cleanup:
	qm_os_memory_free( buf );

	return nullptr;
}
#endif

void ape_room_upload_lightmap_( ApeRoom *self, unsigned int width, unsigned int height )
{
	assert( self->lightmap != nullptr );

	if ( self->lightmapTexture != nullptr )
	{
		ape_texture_release_( self->lightmapTexture );
		ape_memory_flush_unreferenced_resources();
	}

#if defined( APE_RENDERER_LIGHTMAP_USE_FLOATS )
	self->lightmapTexture = ape_texture_generate_( "lightmap", self->lightmap, width, height, &QM_IMAGE_FORMAT_RGB16F_DESC(), PLG_TEXTURE_FILTER_LINEAR );
#else
	self->lightmapTexture = ape_texture_generate_( "lightmap", self->lightmap, width, height, &QM_IMAGE_FORMAT_RGB8_DESC(), PLG_TEXTURE_FILTER_LINEAR );
#endif
	if ( self->lightmapTexture != nullptr )
	{
		//TODO: remove this!!! ITS A BOTCH - this should be updated by the material draw method, probably
		self->lightmapTexture->wrapMode = PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE;
		PlgSetTextureWrapMode( self->lightmapTexture->internal, self->lightmapTexture->wrapMode );
	}
	else
	{
		ape_console_warning_( "Failed to create lightmap texture!\n" );
	}
}

static AcmBranch *ape_room_serialize_( void *self, AcmBranch *root )
{
	ApeRoom *room = self;
	acm_push_ui32( root, "flags", room->flags );
	acm_push_array_f32( root, "ambience", ( float * ) &room->ambientLight, 4 );
	acm_push_ui32( root, "reverb", room->reverbPreset );

	if ( room->lightmap != nullptr )
	{
		AcmBranch *lightmapArray = acm_push_array_f16( root, "lightmap", nullptr, 0 );
		for ( unsigned int i = 0; i < APE_LIGHTMAP_PIXELS; ++i )
		{
			for ( unsigned int j = 0; j < 3; ++j )
			{
				acm_push_f16( lightmapArray, nullptr, room->lightmap[ i ].colour.v[ j ] );
			}
		}
	}

	ape_decal_manager_serialize_( room->decalManager, root );

	return root;
}

static ApeWorldNode *ape_room_deserialize_( ApeWorldNode *self, ApeWorldNode *parent, AcmBranch *root )
{
	ApeRoom *room      = ( ApeRoom * ) self;
	room->flags        = ACM_GET_INT( room->flags, root, "flags", 0 );
	room->ambientLight = com_acm_get_colour_f32( root, "ambience", &QM_MATH_COLOUR4F( 0.0f, 0.0f, 0.0f, 1.0f ) );
	room->reverbPreset = ACM_GET_INT( room->flags, root, "reverb", 0 );

	AcmBranch *lightmapArray = acm_get_child_by_name( root, "lightmap" );
	if ( lightmapArray != nullptr )
	{
		room->lightmap = QM_OS_MEMORY_NEW_( ApeLightmapPixel, APE_LIGHTMAP_PIXELS );

		AcmBranch *child = acm_get_first_child( lightmapArray );
		for ( unsigned int i = 0; i < APE_LIGHTMAP_PIXELS; ++i )
		{
			assert( child != nullptr );
			for ( unsigned int j = 0; j < 3; ++j, child = acm_get_next_child( child ) )
			{
				acm_branch_get_float16( child, &room->lightmap[ i ].colour.v[ j ] );
			}
		}

		ape_room_upload_lightmap_( room, APE_LIGHTMAP_WIDTH, APE_LIGHTMAP_HEIGHT );
	}

	ape_decal_manager_deserialize_( room->decalManager, root );

	ape_world_node_mark_dirty_( self );

	return self;
}

static constexpr unsigned int RAY_HIT_INC = 256;

static bool intersect_ray_children( ApeRoom *self, ApeWorldNode *node, const PLCollisionRay *ray, ApeCollisionIntersection *hits, unsigned int *numHits, unsigned int *maxHits )
{
	if ( node->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		QmMathVector3f intersection;
		if ( !com_collision_ray_intersect_aabb( ray, &node->bounds, &intersection ) )
		{
			return false;
		}

		switch ( node->type )
		{
			default:
				break;
			case APE_WORLD_NODE_TYPE_BRUSH:
			{
				ApeBrush *brush = ( ApeBrush * ) node;
				for ( unsigned int i = 0; i < brush->numFaces; ++i )
				{
					ApeBrushFace *face = &brush->faces[ i ];
					if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN )
					{
						continue;
					}

					QmMathVector3f vertices[ APE_BRUSH_MAX_FACE_VERTICES ];
					for ( unsigned int j = 0; j < face->numVertices; ++j )
					{
						vertices[ j ] = brush->vertices[ face->vertices[ face->edgeLoopOrder[ j ] ].posIndex ];
					}

					if ( !com_collision_ray_intersect_polygon( ray, vertices, face->numVertices, &intersection ) )
					{
						continue;
					}

					ApeCollisionIntersection *hit = &hits[ *numHits ];
					hit->node                     = APE_WORLD_NODE( brush );
					hit->face                     = face;
					hit->intersection             = intersection;
					hit->distance                 = qm_math_vector3f_length( qm_math_vector3f_sub( intersection, ray->origin ) );
					( *numHits )++;
				}
				break;
			}
		}

		if ( *numHits >= *maxHits )
		{
			*maxHits = *maxHits + RAY_HIT_INC;
			hits     = qm_os_memory_realloc( hits, sizeof( ApeCollisionIntersection ) * *maxHits );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		intersect_ray_children( self, child, ray, hits, numHits, maxHits );
	}

	return true;
}

bool ape_room_ray_intersect( ApeRoom *self, const PLCollisionRay *ray, ApeCollisionIntersection *result )
{
	unsigned int              maxHits = RAY_HIT_INC;
	unsigned int              numHits = 0;
	ApeCollisionIntersection *hits    = QM_OS_MEMORY_NEW_( ApeCollisionIntersection, maxHits );

	if ( !intersect_ray_children( self, &self->base, ray, hits, &numHits, &maxHits ) || numHits == 0 )
	{
		qm_os_memory_free( hits );
		return false;
	}

	// now determine which was the closest hit;
	*result = hits[ 0 ];
	for ( unsigned int i = 1; i < numHits; i++ )
	{
		if ( hits[ i ].distance < result->distance )
		{
			*result = hits[ i ];
		}
	}

	qm_os_memory_free( hits );

	return true;
}

static ApeCollisionIntersection *intersect( const ApeCollisionCollider *a, const ApeCollisionCollider *b, ApeCollisionIntersection *result )
{
	switch ( a->type )
	{
		default:
			return nullptr;
		case APE_COLLISION_TYPE_AABB:
			switch ( b->type )
			{
				default:
					return nullptr;
				case APE_COLLISION_TYPE_AABB:
					if ( !com_collision_aabb_intersect_aabb( a->aabb, b->aabb, &result->intersection ) )
					{
						return nullptr;
					}
					break;
				case APE_COLLISION_TYPE_SPHERE:
					if ( !com_collision_sphere_intersect_aabb( b->sphere, a->aabb, &result->intersection ) )
					{
						return nullptr;
					}
					break;
			}
			break;
		case APE_COLLISION_TYPE_SPHERE:
			switch ( b->type )
			{
				default:
					return nullptr;
				case APE_COLLISION_TYPE_AABB:
					if ( !com_collision_sphere_intersect_aabb( a->sphere, b->aabb, &result->intersection ) )
					{
						return nullptr;
					}
					break;
				case APE_COLLISION_TYPE_SPHERE:
					if ( !com_collision_sphere_intersect_sphere( a->sphere, b->sphere, &result->intersection ) )
					{
						return nullptr;
					}
					break;
			}
			break;
	}

	return result;
}

static bool intersect_children( ApeRoom *self, ApeWorldNode *node, const ApeCollisionCollider *collider, ApeCollisionIntersection *hits, unsigned int *numHits, unsigned int *maxHits )
{
	if ( node->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		QmMathVector3f intersection;
		switch ( collider->type )
		{
			default:
				break;
			case APE_COLLISION_TYPE_AABB:
				if ( !com_collision_aabb_intersect_aabb( collider->aabb, &node->bounds, &intersection ) )
				{
					return false;
				}
				break;
			case APE_COLLISION_TYPE_SPHERE:
				if ( !com_collision_sphere_intersect_aabb( collider->sphere, &node->bounds, &intersection ) )
				{
					return false;
				}
				break;
		}

		switch ( node->type )
		{
			default:
				break;
			case APE_WORLD_NODE_TYPE_BRUSH:
			{
				ApeBrush *brush = ( ApeBrush * ) node;
				for ( unsigned int i = 0; i < brush->numFaces; ++i )
				{
					ApeBrushFace *face = &brush->faces[ i ];
					if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN )
					{
						continue;
					}

					QmMathVector3f vertices[ APE_BRUSH_MAX_FACE_VERTICES ];
					for ( unsigned int j = 0; j < face->numVertices; ++j )
					{
						vertices[ j ] = brush->vertices[ face->vertices[ face->edgeLoopOrder[ j ] ].posIndex ];
					}

					ApeCollisionIntersection *hit = &hits[ *numHits ];

					QmMathVector3f normal = qm_math_vector3f_normalize( face->normal );
					if ( collider->type == APE_COLLISION_TYPE_CAPSULE )
					{
						if ( !com_collision_capsule_intersect_polygon( collider->capsule, &normal, vertices, face->numVertices, &intersection ) )
						{
							continue;
						}

						hit->origin = collider->capsule->origin;
					}
					else if ( collider->type == APE_COLLISION_TYPE_SPHERE )
					{
						if ( !com_collision_sphere_intersect_polygon( collider->sphere, &normal, vertices, face->numVertices, &intersection ) )
						{
							continue;
						}

						hit->distance = qm_math_vector3f_length( qm_math_vector3f_sub( intersection, collider->sphere->origin ) );
						hit->depth    = collider->sphere->radius - hit->distance;
						hit->origin   = collider->sphere->origin;
					}
					else if ( collider->type == APE_COLLISION_TYPE_AABB )
					{
						if ( !com_collision_aabb_intersect_polygon( collider->aabb, &normal, vertices, face->numVertices, &intersection ) )
						{
							continue;
						}

						hit->origin = collider->aabb->origin;
					}

					hit->node         = APE_WORLD_NODE( brush );
					hit->face         = face;
					hit->intersection = intersection;
					( *numHits )++;
				}
				break;
			}
		}

		if ( *numHits >= *maxHits )
		{
			*maxHits = *maxHits + RAY_HIT_INC;
			hits     = qm_os_memory_realloc( hits, sizeof( ApeCollisionIntersection ) * *maxHits );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		intersect_children( self, child, collider, hits, numHits, maxHits );
	}

	return true;
}

ApeCollisionIntersection *ape_room_intersect( ApeRoom *self, const ApeCollisionCollider *collider, unsigned int *numHits )
{
	unsigned int maxHits           = RAY_HIT_INC;
	*numHits                       = 0;
	ApeCollisionIntersection *hits = QM_OS_MEMORY_NEW_( ApeCollisionIntersection, maxHits );

	if ( !intersect_children( self, &self->base, collider, hits, numHits, &maxHits ) || numHits == 0 )
	{
		qm_os_memory_free( hits );
		return nullptr;
	}

#if 0
	// now determine which was the closest hit;
	*result = hits[ 0 ];
	for ( unsigned int i = 1; i < numHits; i++ )
	{
		if ( hits[ i ].distance < result->distance )
		{
			*result = hits[ i ];
		}
	}

	qm_os_memory_free( hits );
#endif

	return hits;
}

/////////////////////////////////////////////////////////////////////////////////////

QmMathVector3f ape_room_get_gravity( const ApeRoom *self )
{
	return qm_math_vector3f_add( self->gravity, ape_config_.world.gravityModifier );
}

bool ape_room_create_projected_decal( ApeRoom *self, ApeMaterial *material, const QmMathVector3f *pos, const QmMathVector3f *dir, float angle, float scale )
{
	return ape_decal_manager_create_projected_decal_( self->decalManager, self, material, pos, dir, angle, scale ) != nullptr;
}

static ApePropertyEnum reverbPresetsEnum[] = {
        {"None",             0 },
        {"Forest",           1 },
        {"Default",          2 },
        {"Generic",          3 },
        {"Padded Cell",      4 },
        {"Room",             5 },
        {"Bathroom",         6 },
        {"Living Room",      7 },
        {"Stone Room",       8 },
        {"Auditorium",       9 },
        {"Concert Hall",     10},
        {"Cave",             11},
        {"Arena",            12},
        {"Hangar",           13},
        {"Carpeted Hallway", 14},
        {"Hallway",          15},
        {"Stone Corridor",   16},
        {"Alley",            17},
        {"City",             18},
        {"Mountains",        19},
        {"Quarry",           20},
        {"Plain",            21},
        {"Parking Lot",      22},
        {"Sewer Pipe",       23},
        {"Underwater",       24},
        {"Small Room",       25},
        {"Medium Room",      26},
        {"Large Room",       27},
        {"Medium Hall",      28},
        {"Large Hall",       29},
        {"Plate",            30},
};

static const ApeProperty properties[] = {
        APE_PROPERTY_BASIC( "Ambience", "Set the ambient light level.", ApeRoom, ambientLight, COLOUR ),
        APE_PROPERTY_ENUM( "Reverb", "Type of reverb to fallback to for the given room.", ApeRoom, reverbPreset, reverbPresetsEnum ),
};

const ApeWorldNodeClass ape_roomClass = {
        .identifier = "room",
        .magic      = QM_OS_MAGIC_TO_NUM( 'R', 'O', 'O', 'M' ),

        .create      = create_room,
        .destroy     = destroy_room,
        .serialize   = ape_room_serialize_,
        .deserialize = ape_room_deserialize_,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),

        .flags = APE_WORLD_NODE_CLASS_FLAG_NO_EDITOR,
};
