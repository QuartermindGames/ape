// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include <plcore/pl_hashtable.h>

#include "world/world.h"
#include "ape/ape_public_game.h"

#include "yin/core_game.h"

ApeRoom *ape_room_create( ApeWorldNode *parent, const char *name )
{
	ApeRoom *room = PL_NEW( ApeRoom );
	ape_world_node_setup_( &room->base, parent, APE_WORLD_NODE_TYPE_ROOM, name, &pl_vecOrigin3, &pl_vecOrigin3 );

	// assign the room a random colour so it can be identified per debugging
	room->colour = PL_COLOURF32RGB( PlUniform0To1Random(),
	                                PlUniform0To1Random(),
	                                PlUniform0To1Random() );

	room->gravity = PL_VECTOR3( 0.0f, -0.9f, 0.0f );

	room->taggedSurfaceLookup = PlCreateHashTable();

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

	PL_DELETE( self );
}

void ape_room_set_ambience( ApeRoom *self, PLColourF32 ambience )
{
	self->ambientLight = ambience;
}

PLColourF32 ape_room_get_ambience( const ApeRoom *self )
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
		ape_warning_( "Attempted to add duplicate surface tag (%s)!\n", face->tag );
		return;
	}

	PRINT_DEBUG( "Added \"%s\" to room lookup\n", face->tag );
}

void ape_room_remove_tagged_surface( ApeRoom *self, ApeBrushFace *face )
{
	PLHashTableNode *node = PlLookupHashTableNode( self->taggedSurfaceLookup, face->tag, strlen( face->tag ) );
	if ( node == nullptr )
	{
		ape_warning_( "Failed to remove tag (%s), lookup failed!\n", face->tag );
		return;
	}

	PlDestroyHashTableNode( node );

	PRINT_DEBUG( "Removed \"%s\" from room lookup\n", face->tag );
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

static AcmBranch *ape_room_serialize_( void *self, AcmBranch *root )
{
	ApeRoom *room = self;
	acm_push_ui32( root, "flags", room->flags );
	acm_push_array_f32( root, "colour", ( float * ) &room->colour, 4 );
	acm_push_array_f32( root, "ambience", ( float * ) &room->ambientLight, 4 );
	acm_push_ui32( root, "reverb", room->reverbPreset );

	return root;
}

static ApeWorldNode *ape_room_deserialize_( ApeWorldNode *parent, AcmBranch *root )
{
	ApeRoom *self      = ape_room_create( parent, "temp" );
	self->flags        = ACM_GET_INT( self->flags, root, "flags", 0 );
	self->colour       = com_acm_get_colour_f32( root, "colour", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f ) );
	self->ambientLight = com_acm_get_colour_f32( root, "ambience", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f ) );
	self->reverbPreset = ACM_GET_INT( self->flags, root, "reverb", 0 );

	ape_world_node_mark_dirty_( APE_WORLD_NODE( self ) );

	return &self->base;
}

static constexpr unsigned int RAY_HIT_INC = 256;

static bool intersect_ray_children( ApeRoom *self, ApeWorldNode *node, const PLCollisionRay *ray, ApeCollisionIntersection *hits, unsigned int *numHits, unsigned int *maxHits )
{
	if ( node->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		PLVector3 intersection;
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

					PLVector3 vertices[ APE_BRUSH_MAX_FACE_VERTICES ];
					for ( unsigned int j = 0; j < face->numVertices; ++j )
					{
						vertices[ j ] = *face->edgeLoop[ j ]->position;
					}

					if ( !com_collision_ray_intersect_polygon( ray, vertices, face->numVertices, &intersection ) )
					{
						continue;
					}

					ApeCollisionIntersection *hit = &hits[ *numHits ];
					hit->node                     = APE_WORLD_NODE( brush );
					hit->face                     = face;
					hit->intersection             = intersection;
					hit->distance                 = PlVector3Length( PlSubtractVector3( intersection, ray->origin ) );
					( *numHits )++;
				}
				break;
			}
		}

		if ( *numHits >= *maxHits )
		{
			*maxHits = *maxHits + RAY_HIT_INC;
			hits     = PL_REALLOCA( hits, sizeof( ApeCollisionIntersection ) * *maxHits );
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
	ApeCollisionIntersection *hits    = PL_NEW_( ApeCollisionIntersection, maxHits );

	if ( !intersect_ray_children( self, &self->base, ray, hits, &numHits, &maxHits ) || numHits == 0 )
	{
		PL_DELETE( hits );
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

	PL_DELETE( hits );

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
		PLVector3 intersection;
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

					PLVector3 vertices[ APE_BRUSH_MAX_FACE_VERTICES ];
					for ( unsigned int j = 0; j < face->numVertices; ++j )
					{
						vertices[ j ] = *face->edgeLoop[ j ]->position;
					}

					ApeCollisionIntersection *hit = &hits[ *numHits ];

					PLVector3 normal = PlNormalizeVector3( face->normal );
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

						hit->distance = PlVector3Length( PlSubtractVector3( intersection, collider->sphere->origin ) );
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
			hits     = PL_REALLOCA( hits, sizeof( ApeCollisionIntersection ) * *maxHits );
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
	ApeCollisionIntersection *hits = PL_NEW_( ApeCollisionIntersection, maxHits );

	if ( !intersect_children( self, &self->base, collider, hits, numHits, &maxHits ) || numHits == 0 )
	{
		PL_DELETE( hits );
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

	PL_DELETE( hits );
#endif

	return hits;
}

bool ape_room_set_path( ApeRoom *self, const char *path )
{
	if ( PlSetupPath( self->path, false, "%s", path ) == nullptr )
	{
		ape_warning_( "Invalid path provided: %s\n", PlGetError() );
		return false;
	}

	return true;
}

const char *ape_room_get_path( const ApeRoom *self )
{
	return self->path;
}

/////////////////////////////////////////////////////////////////////////////////////

PLVector3 ape_room_get_gravity( const ApeRoom *self )
{
	return PlAddVector3( self->gravity, ape_config_.world.gravityModifier );
}

#if !defined( APE_NO_EDITOR )

bool ape_room_set_save_path( ApeRoom *self, const char *path )
{
	if ( PlSetupPath( self->savePath, false, "%s", path ) == nullptr )
	{
		ape_warning_( "Invalid path provided: %s\n", PlGetError() );
		return false;
	}

	return true;
}

const char *ape_room_get_save_path( const ApeRoom *self )
{
	if ( *self->savePath == '\0' )
	{
		return nullptr;
	}

	return self->savePath;
}

#endif

static ApeWorldNodePropertyEnum reverbPresetsEnum[] = {
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

static const ApeWorldNodeProperty properties[] = {
        APE_WORLD_NODE_PROPERTY_BASIC( "Ambience", "Set the ambient light level.", ApeRoom, ambientLight, COLOUR ),
        APE_WORLD_NODE_PROPERTY_ENUM( "Reverb", "Type of reverb to fallback to for the given room.", ApeRoom, reverbPreset, reverbPresetsEnum ),
};

const ApeWorldNodeClass ape_roomClass = {
        .identifier = "room",
        .magic      = PL_MAGIC_TO_NUM( 'R', 'O', 'O', 'M' ),

        .destroyFunction     = destroy_room,
        .serializeFunction   = ape_room_serialize_,
        .deserializeFunction = ape_room_deserialize_,

        .properties    = properties,
        .numProperties = PL_ARRAY_ELEMENTS( properties ),

        .flags = APE_WORLD_NODE_CLASS_FLAG_NO_EDITOR,
};
