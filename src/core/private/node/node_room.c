// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "../world/world.h"

ApeRoom *ape_room_create( ApeWorldNode *parent, const char *name )
{
	ApeRoom *room = PL_NEW( ApeRoom );
	ape_world_node_setup_( &room->base, parent, APE_WORLD_NODE_TYPE_ROOM, name, &pl_vecOrigin3, &pl_vecOrigin3 );

	room->subRooms = PlCreateVectorArray( 0 );
	room->faces    = PlCreateVectorArray( 0 );
	room->portals  = PlCreateVectorArray( 0 );

	// assign the room a random colour so it can be identified per debugging
	room->colour = PL_COLOURF32RGB( PlUniform0To1Random(),
	                                PlUniform0To1Random(),
	                                PlUniform0To1Random() );

	return room;
}

static void destroy_room( void *data, ApeWorldNode *parent )
{
	ApeRoom *self = data;

	PlDestroyVectorArray( self->subRooms );
	PlDestroyVectorArray( self->faces );
	PlDestroyVectorArray( self->portals );

	PlDestroyLinkedList( self->lights );

	PlgDestroyMesh( self->mesh );

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

AcmBranch *ape_room_serialize_( void *self, AcmBranch *root )
{
	ApeRoom *room = self;
	acm_push_string( root, "path", room->path, true );
	acm_push_ui32( root, "flags", room->flags );
	acm_push_array_f32( root, "colour", ( float * ) &room->colour, 4 );
	acm_push_array_f32( root, "ambience", ( float * ) &room->ambientLight, 4 );
	acm_push_ui32( root, "reverb", room->reverbPreset );

	return root;
}

ApeWorldNode *ape_room_deserialize_( ApeWorldNode *parent, AcmBranch *root )
{
	ApeRoom *self = ape_room_create( parent, "temp" );

	const char *path = acm_get_string( root, "path", nullptr );
	if ( path != nullptr )
	{
		snprintf( self->path, sizeof( self->path ), "%s", path );
	}

	self->flags        = ACM_GET_INT( self->flags, root, "flags", 0 );
	self->colour       = com_acm_get_colour_f32( root, "colour", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f ) );
	self->ambientLight = com_acm_get_colour_f32( root, "ambience", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f ) );
	self->reverbPreset = ACM_GET_INT( self->flags, root, "reverb", 0 );

	self->isDirty = true;

	return &self->base;
}

static constexpr UInt RAY_HIT_INC = 256;

static bool intersect_ray_children( ApeRoom *self, ApeWorldNode *node, const PLCollisionRay *ray, ApeRayIntersection *hits, UInt *numHits, UInt *maxHits )
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
				for ( UInt i = 0; i < brush->numFaces; ++i )
				{
					ApeBrushFace *face = &brush->faces[ i ];

					PLVector3 vertices[ APE_BRUSH_MAX_FACE_VERTICES ];
					for ( uint j = 0; j < face->numVertices; ++j )
					{
						vertices[ j ] = *face->edgeLoop[ j ]->position;
					}

					if ( !com_collision_ray_intersect_polygon( ray, vertices, face->numVertices, &intersection ) )
					{
						continue;
					}

					ApeRayIntersection *hit = &hits[ *numHits ];
					hit->node               = APE_WORLD_NODE( brush );
					hit->face               = face;
					hit->intersection       = intersection;
					hit->distance           = PlVector3Length( PlSubtractVector3( intersection, ray->origin ) );
					( *numHits )++;
				}
				break;
			}
		}

		if ( *numHits >= *maxHits )
		{
			*maxHits = *maxHits + RAY_HIT_INC;
			hits     = PL_REALLOCA( hits, sizeof( ApeRayIntersection ) * *maxHits );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		intersect_ray_children( self, child, ray, hits, numHits, maxHits );
	}

	return true;
}

bool ape_room_ray_intersect( ApeRoom *self, const PLCollisionRay *ray, ApeRayIntersection *result )
{
	UInt                maxHits = RAY_HIT_INC;
	UInt                numHits = 0;
	ApeRayIntersection *hits    = PL_NEW_( ApeRayIntersection, maxHits );

	if ( !intersect_ray_children( self, &self->base, ray, hits, &numHits, &maxHits ) || numHits == 0 )
	{
		PL_DELETE( hits );
		return false;
	}

	// now determine which was the closest hit;
	*result = hits[ 0 ];
	for ( UInt i = 1; i < numHits; i++ )
	{
		if ( hits[ i ].distance < result->distance )
		{
			*result = hits[ i ];
		}
	}

	PL_DELETE( hits );

	return true;
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

static const ApeWorldNodePropertyEnum reverbPresetsEnum[] = {
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
