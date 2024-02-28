// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"

#include "world.h"
#include "client/renderer/renderer.h"

/****************************************
 * PRIVATE
 ****************************************/

static ApeLight *deserialize_light( NdBranch *root )
{
	ApeLight *light = PL_NEW( ApeLight );

	light->position = ndGetVector3( root, "position", &pl_vecOrigin3 );
	light->angles = ndGetVector3( root, "angles", &pl_vecOrigin3 );

	light->colour = ndGetColourF32( root, "colour", &PL_COLOURF32_WHITE );
	light->radius = ndGetF32ByName( root, "radius", 0.0f );

	light->isHidden = ndGetBoolByName( root, "isHidden", false );
	light->flags = ndGetUInt( root, "flags", 0 );

	return light;
}

static void deserialize_lights( ApeWorld *world, NdBranch *root )
{
	unsigned int numLights = ndGetNumOfChildren( root );
	if ( numLights == 0 )
	{
		return;
	}

	PlResizeVectorArray( world->lights, numLights );

	NdBranch *child = ndGetFirstChild( root );
	while ( child != NULL )
	{
		PlPushBackVectorArrayElement( world->lights, deserialize_light( child ) );
		child = ndGetNextChild( child );
	}
}

static ApeWorldRoom *deserialize_room( ApeWorld *world, NdBranch *root )
{
	ApeWorldRoom *room = ape_world_room_create();

	room->bounds.mins = ndGetVector3( root, "mins", &pl_vecOrigin3 );
	room->bounds.maxs = ndGetVector3( root, "maxs", &pl_vecOrigin3 );

	room->isDetail = ndGetBoolByName( root, "isDetail", false );
	room->ambientLight = ndGetColourF32( root, "ambience", &PL_COLOURF32_BLACK );
	room->flags = ndGetUInt( root, "flags", 0 );

	char tmp[ 64 ];
	snprintf( tmp, sizeof( tmp ), "room_%u", PlGetNumVectorArrayElements( world->rooms ) );

	room->worldNode = ape_world_node_create( world->root, tmp, APE_WORLD_NODE_TYPE_ROOM, room );

	return room;
}

static ApeWorldPortal *deserialize_portal( ApeWorld *world, NdBranch *root )
{
	// Fetch the first room index and validate it
	ApeWorldRoom *roomA = PlGetVectorArrayElementAt( world->rooms, ndGetUInt( root, "roomB", ( unsigned int ) -1 ) );
	assert( roomA != NULL );
	if ( roomA == NULL )
	{
		PRINT_WARNING( "Invalid portal room A!\n" );
		return NULL;
	}

	// Fetch the second room index and validate it
	ApeWorldRoom *roomB = PlGetVectorArrayElementAt( world->rooms, ndGetUInt( root, "roomA", ( unsigned int ) -1 ) );
	assert( roomB != NULL );
	if ( roomB == NULL )
	{
		PRINT_WARNING( "Invalid portal room B!\n" );
		return NULL;
	}

	ApeWorldPortal *portal = PL_NEW( ApeWorldPortal );

	portal->mins = ndGetVector3( root, "mins", &pl_vecOrigin3 );
	portal->maxs = ndGetVector3( root, "maxs", &pl_vecOrigin3 );

	// Get the two associated rooms for the portal
	portal->roomA = roomA;
	PlPushBackVectorArrayElement( portal->roomA->portals, portal );
	portal->roomB = roomB;
	PlPushBackVectorArrayElement( portal->roomB->portals, portal );

	PlPushBackVectorArrayElement( world->portals, portal );

	return portal;
}

static ApeWorldFace *deserialize_face( ApeWorld *world, NdBranch *root )
{
	ApeWorldFace *face = PL_NEW( ApeWorldFace );

	face->normal = ndGetVector3( root, "normal", &pl_vecOrigin3 );
	face->offset = ndGetF32ByName( root, "offset", 0.0f );

	face->flags = ndGetUInt( root, "flags", 0 );

	face->smoothingGroup = ndGetInt( root, "smoothingGroup", 0 );

	unsigned int roomIndex = ndGetUInt( root, "roomIndex", ( unsigned int ) -1 );
	assert( roomIndex != ( unsigned int ) -1 );
	if ( roomIndex == ( unsigned int ) -1 )
		PRINT_WARNING( "No room index for face!\n" );
	else
	{
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );
		if ( room == NULL )
			PRINT_WARNING( "Invalid room index (%u) for face!\n", roomIndex );
		else
			PlPushBackVectorArrayElement( room->faces, face );
	}

	// Attempt to fetch the material for the face.
	// It's inherited from our adventures with RFL, but we'll support cases where a
	// face doesn't have a material as "valid"...
	face->materialIndex = ndGetUInt( root, "material", ( unsigned int ) -1 );
	face->material = PlGetVectorArrayElementAt( world->materials, face->materialIndex );
	assert( face->material != NULL );
	if ( face->material == NULL )
		face->material = ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_FALLBACK );

	face->edgeLoop = PlCreateLinkedList();
	face->vertices = PlCreateVectorArray( 0 );

	// Each face has a collection of vertices, the position of which is stored
	// per the world vertex list rather than for the face itself, which is
	// inherited, again, from our adventures with RFL (but not necessarily bad)
	NdBranch *branch;
	if ( ( branch = ndGetChildByName( root, "edges" ) ) != NULL )
	{
		branch = ndGetFirstChild( branch );
		while ( branch != NULL )
		{
			ApeWorldVertex *worldVertex;
			unsigned int vertexIndex = ndGetUInt( branch, "vertexIndex", ( unsigned int ) -1 );
			assert( vertexIndex != ( unsigned int ) -1 );
			if ( ( worldVertex = PlGetVectorArrayElementAt( world->vertices, vertexIndex ) ) == NULL )
			{
				PRINT_WARNING( "Invalid vertex index for face!\n" );
				break;
			}

			ApeWorldFaceVertex *vertex = PL_NEW( ApeWorldFaceVertex );
			vertex->u = worldVertex;
			vertex->uv = ndGetVector2( branch, "uv", &pl_vecOrigin2 );
			vertex->normal = ndGetVector3( branch, "normal", &pl_vecOrigin3 );

			PlInsertLinkedListNode( face->edgeLoop, vertex );
			PlPushBackVectorArrayElement( face->vertices, vertex );

			branch = ndGetNextChild( branch );
		}
	}

	ape_world_face_generate_bounds( face );
	face->origin = pl_vecOrigin3;//HACK ...

	return face;
}

static void deserialize_geometry( ApeWorld *world, NdBranch *root )
{
	NdBranch *branch;

	if ( ( branch = ndGetChildByName( root, "materials" ) ) != NULL )
	{
		unsigned int numMaterials = ndGetNumOfChildren( branch );
		if ( numMaterials > 0 )
		{
			if ( world->materials == NULL )
			{
				world->materials = PlCreateVectorArray( numMaterials );
			}

			branch = ndGetFirstChild( branch );
			while ( branch != NULL )
			{
				PLPath path;
				if ( ndGetStr( branch, path, sizeof( path ) ) == ND_ERROR_SUCCESS )
				{
					PlPushBackVectorArrayElement( world->materials, ss_arl_material_cache( path, APE_CACHE_WORLD, true, false ) );
				}
				else
				{
					PRINT_WARNING( "Failed to fetch string from materials list: %s\n", ndGetErrorMessage() );
					break;
				}

				branch = ndGetNextChild( branch );
			}
		}
		else
		{
			PRINT_WARNING( "No materials for geometry!\n" );
		}
	}

	if ( ( branch = ndGetChildByName( root, "rooms" ) ) != NULL )
	{
		unsigned int numRooms = ndGetNumOfChildren( branch );
		if ( numRooms > 0 )
		{
			if ( world->rooms == NULL )
			{
				world->rooms = PlCreateVectorArray( numRooms );
			}

			branch = ndGetFirstChild( branch );
			while ( branch != NULL )
			{
				PlPushBackVectorArrayElement( world->rooms, deserialize_room( world, branch ) );
				branch = ndGetNextChild( branch );
			}
		}
		else
		{
			PRINT_WARNING( "No rooms for geometry!\n" );
		}
	}

	if ( ( branch = ndGetChildByName( root, "portals" ) ) != NULL )
	{
		unsigned int numPortals = ndGetNumOfChildren( branch );
		if ( numPortals > 0 )
		{
			if ( world->portals == NULL )
			{
				world->portals = PlCreateVectorArray( numPortals );
			}

			NdBranch *child = ndGetFirstChild( branch );
			while ( child != NULL )
			{
				ApeWorldPortal *portal = deserialize_portal( world, child );
				if ( portal != NULL )
				{
					PlPushBackVectorArrayElement( world->portals, portal );
				}

				child = ndGetNextChild( child );
			}
		}
		else
		{
			PRINT_WARNING( "No portals for geometry!\n" );
		}
	}

	// Attempt to fetch the list of vertices - these are just an immediate
	// list of coordinates
	if ( ( branch = ndGetChildByName( root, "vertices" ) ) != NULL )
	{
		// Vertices should be just a big ol' list of floats
		// representing x y z coordinates
		unsigned int numChildren = ndGetNumOfChildren( branch );
		if ( numChildren % 3 == 0 )
		{
			unsigned int numVertices = numChildren / 3;
			if ( numVertices > 0 )
			{
				if ( world->vertices == NULL )
					world->vertices = PlCreateVectorArray( numVertices );

				float *vertices = PL_NEW_( float, numChildren );
				ndGetF32Array( branch, vertices, numChildren );
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
			PRINT_WARNING( "Invalid number of vertices for geometry!\n" );
		}
	}

	if ( ( branch = ndGetChildByName( root, "faces" ) ) != NULL )
	{
		unsigned int numFaces = ndGetNumOfChildren( branch );
		if ( numFaces > 0 )
		{
			branch = ndGetFirstChild( branch );
			while ( branch != NULL )
			{
				deserialize_face( world, branch );
				branch = ndGetNextChild( branch );
			}
		}
		else
		{
			PRINT_WARNING( "No faces for geometry!\n" );
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
	ApeWorld *world = ape_world_create();
	if ( world == NULL )
	{
		PRINT_WARNING( "Failed to create world!\n" );
		return NULL;
	}

	// Get the world version from the branch
	unsigned int version = ndGetUInt( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 )
	{
		// Print a warning and return NULL if the version is not found
		PRINT_WARNING( "Failed to find world version!\n" );
		return NULL;
	}
	else if ( version > APE_WORLD_VERSION )
	{
		// Print a warning and return NULL if the version is not supported
		PRINT_WARNING( "Unsupported world version! (%d > %d)\n", version, APE_WORLD_VERSION );
		return NULL;
	}

	NdBranch *branch;

	// Get the property branch from the root
	if ( ( branch = ndGetChildByName( root, "properties" ) ) != NULL )
	{
		// Copy the branch, so we can pass it over to the game logic later
		world->globalProperties = ndCopyBranch( branch );

		// Get the global properties of the world from the branch

		world->ambience = ndGetColourF32( world->globalProperties, "ambience", &WORLD_DEFAULT_AMBIENCE );

		world->clearColour = ndGetColourF32( world->globalProperties, "clearColour", &WORLD_DEFAULT_CLEARCOLOUR );

		world->fogColour = ndGetColourF32( world->globalProperties, "fogColour", &WORLD_DEFAULT_CLEARCOLOUR );
		world->fogFar = ndGetF32ByName( world->globalProperties, "fogFar", 4.0f );
		world->fogNear = ndGetF32ByName( world->globalProperties, "fogNear", 0.01f );
	}

	if ( ( branch = ndGetChildByName( root, "geometry" ) ) != NULL )
		deserialize_geometry( world, branch );

	if ( ( branch = ndGetChildByName( root, "lights" ) ) != NULL )
		deserialize_lights( world, branch );

	return world;
}
