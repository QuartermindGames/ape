// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"

#include "world.h"
#include "client/renderer/renderer.h"

/****************************************
 * PRIVATE
 ****************************************/

static void deserialize_light( ApeWorld *world, NdBranch *root )
{
	ApeWorldNode *worldNode = ape_world_get_world_node( world );

	PLVector3 position = nd_get_vector3( root, "position", &pl_vecOrigin3 );
	PLColourF32 colour = nd_get_colour_f32( root, "colour", &PL_COLOURF32_WHITE );
	ApeLight *light = ape_create_light( worldNode, &position, &colour,
	                                    nd_branch_get_child_float32( root, "radius", 0.0f ),
	                                    nd_branch_get_child_uint( root, "type", APE_LIGHT_TYPE_OMNI ),
	                                    nd_branch_get_child_uint( root, "flags", 0 ) );

	PLVector3 angles = nd_get_vector3( root, "angles", &pl_vecOrigin3 );
	ape_light_set_angles( light, &angles );

	light->isHidden = nd_branch_get_child_bool( root, "isHidden", false );
}

static void deserialize_lights( ApeWorld *world, NdBranch *root )
{
	NdBranch *child = nd_branch_get_first_child( root );
	while ( child != nullptr )
	{
		deserialize_light( world, child );
		child = nd_get_next_child( child );
	}
}

static ApeRoom *deserialize_room( ApeWorld *world, NdBranch *root )
{
	ApeRoom *room = ape_create_room( world->root );

	PLVector3 mins = nd_get_vector3( root, "mins", &pl_vecOrigin3 );
	PLVector3 maxs = nd_get_vector3( root, "maxs", &pl_vecOrigin3 );
	ape_world_node_set_local_bounds( room->header.node, &mins, &maxs );

	room->isDetail = nd_branch_get_child_bool( root, "isDetail", false );
	room->ambientLight = nd_get_colour_f32( root, "ambience", &PL_COLOURF32_BLACK );
	room->flags = nd_branch_get_child_uint( root, "flags", 0 );

	char tmp[ 64 ];
	snprintf( tmp, sizeof( tmp ), "room_%u", PlGetNumVectorArrayElements( world->rooms ) );

	return room;
}

static ApeWorldFace *deserialize_face( ApeWorld *world, NdBranch *root )
{
	ApeWorldFace *face = PL_NEW( ApeWorldFace );

	face->normal = nd_get_vector3( root, "normal", &pl_vecOrigin3 );
	face->offset = nd_branch_get_child_float32( root, "offset", 0.0f );

	face->flags = nd_branch_get_child_uint( root, "flags", 0 );

	unsigned int roomIndex = nd_branch_get_child_uint( root, "roomIndex", ( unsigned int ) -1 );
	if ( roomIndex == ( unsigned int ) -1 )
	{
		ape_warning_( "No room index for face!\n" );
	}
	else
	{
		ApeRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		if ( room == nullptr )
		{
			ape_warning_( "Invalid room index (%u) for face!\n", roomIndex );
		}
		else
		{
			PlPushBackVectorArrayElement( room->faces, face );
		}
	}

	// Attempt to fetch the material for the face.
	// It's inherited from our adventures with RFL, but we'll support cases where a
	// face doesn't have a material as "valid"...
	face->materialIndex = ( int32_t ) nd_branch_get_child_int( root, "material", -1 );
	face->material = PlGetVectorArrayElementAt( world->materials, face->materialIndex );
	if ( face->material == NULL )
	{
		ape_warning_( "Encountered an invalid material index (%u) for world!\n", face->materialIndex );
		face->material = ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_FALLBACK );
	}

	face->edgeLoop = PlCreateLinkedList();
	face->vertices = PlCreateVectorArray( 0 );

	// Each face has a collection of vertices, the position of which is stored
	// per the world vertex list rather than for the face itself, which is
	// inherited, again, from our adventures with RFL (but not necessarily bad)
	NdBranch *branch;
	if ( ( branch = nd_branch_get_child_by_name( root, "edges" ) ) != NULL )
	{
		branch = nd_branch_get_first_child( branch );
		while ( branch != NULL )
		{
			ApeWorldVertex *worldVertex;
			unsigned int vertexIndex = nd_branch_get_child_uint( branch, "vertexIndex", ( unsigned int ) -1 );
			assert( vertexIndex != ( unsigned int ) -1 );
			if ( ( worldVertex = PlGetVectorArrayElementAt( world->vertices, vertexIndex ) ) == NULL )
			{
				ape_warning_( "Invalid vertex index for face!\n" );
				break;
			}

			ApeWorldFaceVertex *vertex = PL_NEW( ApeWorldFaceVertex );
			vertex->u = worldVertex;
			vertex->uv = nd_get_vector2( branch, "uv", &pl_vecOrigin2 );
			vertex->normal = nd_get_vector3( branch, "normal", &pl_vecOrigin3 );

			PlInsertLinkedListNode( face->edgeLoop, vertex );
			PlPushBackVectorArrayElement( face->vertices, vertex );

			branch = nd_get_next_child( branch );
		}
	}

	ape_world_face_generate_bounds( face );
	face->origin = pl_vecOrigin3;//HACK ...

	return face;
}

static void deserialize_geometry( ApeWorld *world, NdBranch *root )
{
	NdBranch *branch;

	if ( ( branch = nd_branch_get_child_by_name( root, "materials" ) ) != NULL )
	{
		unsigned int numMaterials = nd_branch_get_num_of_children( branch );
		if ( numMaterials > 0 )
		{
			if ( world->materials == NULL )
			{
				world->materials = PlCreateVectorArray( numMaterials );
			}

			branch = nd_branch_get_first_child( branch );
			while ( branch != NULL )
			{
				PLPath path;
				if ( nd_branch_get_string( branch, path, sizeof( path ) ) == ND_ERROR_SUCCESS )
				{
					PlPushBackVectorArrayElement( world->materials, ape_material_cache( path, APE_CACHE_GROUP_WORLD, true, false ) );
				}
				else
				{
					ape_warning_( "Failed to fetch string from materials list: %s\n", nd_get_error_message() );
					break;
				}

				branch = nd_get_next_child( branch );
			}
		}
		else
		{
			ape_warning_( "No materials for geometry!\n" );
		}
	}

	if ( ( branch = nd_branch_get_child_by_name( root, "rooms" ) ) != NULL )
	{
		unsigned int numRooms = nd_branch_get_num_of_children( branch );
		if ( numRooms > 0 )
		{
			if ( world->rooms == NULL )
			{
				world->rooms = PlCreateVectorArray( numRooms );
			}

			branch = nd_branch_get_first_child( branch );
			while ( branch != NULL )
			{
				PlPushBackVectorArrayElement( world->rooms, deserialize_room( world, branch ) );
				branch = nd_get_next_child( branch );
			}
		}
		else
		{
			ape_warning_( "No rooms for geometry!\n" );
		}
	}

	// Attempt to fetch the list of vertices - these are just an immediate
	// list of coordinates
	if ( ( branch = nd_branch_get_child_by_name( root, "vertices" ) ) != NULL )
	{
		// Vertices should be just a big ol' list of floats
		// representing x y z coordinates
		unsigned int numChildren = nd_branch_get_num_of_children( branch );
		if ( numChildren % 3 == 0 )
		{
			unsigned int numVertices = numChildren / 3;
			if ( numVertices > 0 )
			{
				if ( world->vertices == NULL )
					world->vertices = PlCreateVectorArray( numVertices );

				float *vertices = PL_NEW_( float, numChildren );
				nd_branch_get_float32_array( branch, vertices, numChildren );
				for ( unsigned int i = 0, j = 0; i < numVertices; ++i, j += 3 )
				{
					ApeWorldVertex *vertex = PL_NEW( ApeWorldVertex );
					vertex->position.x = vertices[ j ];
					vertex->position.y = vertices[ j + 1 ];
					vertex->position.z = vertices[ j + 2 ];
					PlPushBackVectorArrayElement( world->vertices, vertex );
				}

				PL_DELETE( vertices );
			}
		}
		else
		{
			ape_warning_( "Invalid number of vertices for geometry!\n" );
		}
	}

	if ( ( branch = nd_branch_get_child_by_name( root, "faces" ) ) != NULL )
	{
		unsigned int numFaces = nd_branch_get_num_of_children( branch );
		if ( numFaces > 0 )
		{
			branch = nd_branch_get_first_child( branch );
			while ( branch != NULL )
			{
				deserialize_face( world, branch );
				branch = nd_get_next_child( branch );
			}
		}
		else
		{
			ape_warning_( "No faces for geometry!\n" );
		}
	}
}

/****************************************
 * PUBLIC
 ****************************************/

void ape_world_face_generate_bounds( ApeWorldFace *face )
{
	unsigned int numVertices = PlGetNumVectorArrayElements( face->vertices );
	ApeWorldFaceVertex **vertices = ( ApeWorldFaceVertex ** ) PlGetVectorArrayData( face->vertices );
	if ( numVertices == 0 )
		return;

	PLVector3 *boundVertices = PL_NEW_( PLVector3, numVertices );
	for ( unsigned int i = 0; i < numVertices; ++i )
		boundVertices[ i ] = vertices[ i ]->u->position;

	face->bounds = PlGenerateAabbFromCoords( boundVertices, numVertices, true );
	face->origin = PlGetAabbAbsOrigin( &face->bounds, pl_vecOrigin3 );

	PL_DELETE( boundVertices );
}

ApeWorld *ape_world_deserialize_( NdBranch *root )
{
	ApeWorld *world = ape_create_world();
	if ( world == NULL )
	{
		ape_warning_( "Failed to create world!\n" );
		return NULL;
	}

	// Get the world version from the branch
	unsigned int version = nd_branch_get_child_uint( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 )
	{
		// Print a warning and return NULL if the version is not found
		ape_warning_( "Failed to find world version!\n" );
		return NULL;
	}
	else if ( version > APE_WORLD_VERSION )
	{
		// Print a warning and return NULL if the version is not supported
		ape_warning_( "Unsupported world version! (%d > %d)\n", version, APE_WORLD_VERSION );
		return NULL;
	}

	NdBranch *branch;

	// Get the property branch from the root
	if ( ( branch = nd_branch_get_child_by_name( root, "properties" ) ) != NULL )
	{
		// Copy the branch, so we can pass it over to the game logic later
		world->globalProperties = nd_copy_branch( branch );

		// Get the global properties of the world from the branch

		world->ambience = nd_get_colour_f32( world->globalProperties, "ambience", &WORLD_DEFAULT_AMBIENCE );

		world->clearColour = nd_get_colour_f32( world->globalProperties, "clearColour", &WORLD_DEFAULT_CLEARCOLOUR );

		world->fogColour = nd_get_colour_f32( world->globalProperties, "fogColour", &WORLD_DEFAULT_CLEARCOLOUR );
		world->fogFar = nd_branch_get_child_float32( world->globalProperties, "fogFar", 32.0f );
		world->fogNear = nd_branch_get_child_float32( world->globalProperties, "fogNear", 0.01f );
	}

	if ( ( branch = nd_branch_get_child_by_name( root, "geometry" ) ) != NULL )
	{
		deserialize_geometry( world, branch );
	}

	if ( ( branch = nd_branch_get_child_by_name( root, "lights" ) ) != NULL )
	{
		deserialize_lights( world, branch );
	}

	return world;
}
