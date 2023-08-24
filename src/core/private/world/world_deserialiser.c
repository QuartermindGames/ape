// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"

#include "world.h"
#include "client/renderer/renderer.h"

/****************************************
 * PRIVATE
 ****************************************/

static ApeLight *DeserializeLight( NdBranch *root ) {
	ApeLight *light = PL_NEW( ApeLight );

	light->position = ndGetVector3( root, "position", &pl_vecOrigin3 );
	light->angles = ndGetVector3( root, "angles", &pl_vecOrigin3 );

	light->colour = ndGetColourF32( root, "colour", &PL_COLOURF32_WHITE );
	light->radius = ndGetF32ByName( root, "radius", 0.0f );

	light->isHidden = ndGetBoolByName( root, "isHidden", false );
	light->flags = ndGetUInt( root, "flags", 0 );

	return light;
}

static void DeserializeLights( ApeWorld *world, NdBranch *root ) {
	NdBranch *lights = ndGetChildByName( root, "lights" );
	if ( lights == NULL ) {
		return;
	}

	unsigned int numLights = ndGetNumOfChildren( lights );
	if ( numLights == 0 ) {
		return;
	}

	world->lights = PlCreateVectorArray( numLights );

	NdBranch *child = ndGetFirstChild( lights );
	while ( child != NULL ) {
		PlPushBackVectorArrayElement( world->lights, DeserializeLight( child ) );
		child = ndGetNextChild( child );
	}
}

static void DeserializeMaterials( NdBranch *root, ApeWorld *out ) {
	NdBranch *materials = ndGetChildByName( root, "materials" );
	if ( materials == NULL ) {
		return;
	}

	unsigned int numMaterials = ndGetNumOfChildren( materials );
	if ( numMaterials == 0 ) {
		return;
	}

	if ( ndGetType( materials ) != ND_PROPERTY_STRING ) {
		PRINT_WARNING( "Unexpected branch type for materials!\n" );
		return;
	}

	out->materials = PlCreateVectorArray( numMaterials );

	NdBranch *child = ndGetFirstChild( materials );
	while ( child != NULL ) {
		PLPath path;
		if ( ndGetStr( child, path, sizeof( path ) ) == ND_ERROR_SUCCESS )
			PlPushBackVectorArrayElement( out->materials, apeCacheMaterial( path, APE_CACHE_WORLD, true, false ) );
		else
			PRINT_WARNING( "Failed to fetch string from materials list: %s\n", ndGetErrorMessage() );

		child = ndGetNextChild( child );
	}
}

static ApeWorldRoom *DeserializeRoom( NdBranch *root ) {
	ApeWorldRoom *room = apeCreateWorldRoom();

	room->tag = ndGetInt( root, "tag", 0 );

	room->bounds.mins = ndGetVector3( root, "mins", &pl_vecOrigin3 );
	room->bounds.maxs = ndGetVector3( root, "maxs", &pl_vecOrigin3 );

	room->life = ndGetF32ByName( root, "life", 0.0f );

	room->isDetail = ndGetBoolByName( root, "isDetail", false );
	room->ambientLight = ndGetColourF32( root, "ambience", &PL_COLOURF32_BLACK );
	room->flags = ndGetUInt( root, "flags", 0 );

	return room;
}

static void DeserializeRooms( NdBranch *root, ApeWorld *out ) {
	NdBranch *rooms = ndGetChildByName( root, "rooms" );
	if ( rooms == NULL ) {
		return;
	}

	unsigned int numRooms = ndGetNumOfChildren( rooms );
	if ( numRooms == 0 ) {
		return;
	}

	out->rooms = PlCreateVectorArray( numRooms );

	NdBranch *child = ndGetFirstChild( rooms );
	while ( child != NULL ) {
		PlPushBackVectorArrayElement( out->rooms, DeserializeRoom( child ) );
		child = ndGetNextChild( child );
	}
}

static ApeWorldPortal *DeserializePortal( NdBranch *root, ApeWorld *out ) {
	unsigned int numRooms = PlGetNumVectorArrayElements( out->rooms );

	// Fetch the first room index and validate it
	unsigned int roomA = ndGetUInt( root, "roomA", ( unsigned int ) -1 );
	assert( roomA < numRooms );
	if ( roomA >= numRooms ) {
		PRINT_WARNING( "Invalid portal room A (%u)!\n", roomA );
		return NULL;
	}

	// Fetch the second room index and validate it
	unsigned int roomB = ndGetUInt( root, "roomB", ( unsigned int ) -1 );
	assert( roomB < numRooms );
	if ( roomB >= numRooms ) {
		PRINT_WARNING( "Invalid portal room B (%u)!\n", roomB );
		return NULL;
	}

	ApeWorldPortal *portal = PL_NEW( ApeWorldPortal );

	// Get the two associated rooms for the portal
	portal->roomA = PlGetVectorArrayElementAt( out->rooms, roomA );
	PlPushBackVectorArrayElement( portal->roomA->portals, portal );
	portal->roomB = PlGetVectorArrayElementAt( out->rooms, roomB );
	PlPushBackVectorArrayElement( portal->roomB->portals, portal );

	portal->mins = ndGetVector3( root, "mins", &pl_vecOrigin3 );
	portal->maxs = ndGetVector3( root, "maxs", &pl_vecOrigin3 );

	return portal;
}

static void DeserializePortals( NdBranch *root, ApeWorld *out ) {
	// Check if there are some rooms first, if there aren't any then the
	// entire portal list is probably bogus
	unsigned int numRooms = PlGetNumVectorArrayElements( out->rooms );
	if ( numRooms == 0 ) {
		return;
	}

	NdBranch *portals = ndGetChildByName( root, "portals" );
	if ( portals == NULL ) {
		return;
	}

	unsigned int numPortals = ndGetNumOfChildren( portals );
	if ( numPortals == 0 ) {
		return;
	}

	out->portals = PlCreateVectorArray( numPortals );

	NdBranch *child = ndGetFirstChild( portals );
	while ( child != NULL ) {
		ApeWorldPortal *portal = DeserializePortal( child, out );
		if ( portal != NULL ) {
			PlPushBackVectorArrayElement( out->portals, portal );
		}

		child = ndGetNextChild( child );
	}
}

static ApeWorldFace *DeserializeFace( ApeWorld *world, NdBranch *root ) {
	ApeWorldFace *face = PL_NEW( ApeWorldFace );

	face->normal = ndGetVector3( root, "normal", &pl_vecOrigin3 );
	face->offset = ndGetF32ByName( root, "offset", 0.0f );

	face->flags = ND_GETUINT32( root, "flags", 0 );

	face->smoothingGroup = ndGetInt( root, "smoothingGroup", 0 );

	// Attempt to fetch the material for the face.
	// It's inherited from our adventures with RFL, but we'll support cases where a
	// face doesn't have a material as "valid"...
	unsigned int materialIndex = ndGetUInt( root, "material", ( unsigned int ) -1 );
	face->material = PlGetVectorArrayElementAt( world->materials, materialIndex );
	if ( face->material == NULL ) {
		face->material = apeGetDefaultMaterial( APE_MATERIAL_DEFAULT_FALLBACK );
	}

	face->edgeLoop = PlCreateLinkedList();
	face->vertices = PlCreateVectorArray( 0 );

	// Each face has a collection of vertices, the position of which is stored
	// per the world vertex list rather than for the face itself, which is
	// inherited, again, from our adventures with RFL (but not necessarily bad)
	NdBranch *branch;
	if ( ( branch = ndGetChildByName( root, "vertices" ) ) != NULL ) {
		branch = ndGetFirstChild( branch );
		while ( branch != NULL ) {
			ApeWorldVertex *worldVertex;
			unsigned int vertexIndex = ndGetUInt( branch, "vertexIndex", ( unsigned int ) -1 );
			assert( vertexIndex != ( unsigned int ) -1 );
			if ( ( worldVertex = PlGetVectorArrayElementAt( world->vertices, vertexIndex ) ) == NULL ) {
				PRINT_WARNING( "Invalid vertex index for face!\n" );
				break;
			}

			ApeWorldFaceVertex *vertex = PL_NEW( ApeWorldFaceVertex );
			vertex->u = worldVertex;
			vertex->textureU = ndGetF32ByName( branch, "textureU", 0.0f );
			vertex->textureV = ndGetF32ByName( branch, "textureV", 0.0f );

			PlInsertLinkedListNode( face->edgeLoop, vertex );
			PlPushBackVectorArrayElement( face->vertices, vertex );

			branch = ndGetNextChild( branch );
		}
	}

	return face;
}

static void DeserializeGeometry( ApeWorld *world, NdBranch *root ) {
	DeserializeMaterials( root, world );
	DeserializeRooms( root, world );
	DeserializePortals( root, world );

	NdBranch *branch;

	// Attempt to fetch the list of vertices - these are just an immediate
	// list of coordinates
	if ( ( branch = ndGetChildByName( root, "vertices" ) ) != NULL ) {
		// Vertices should be just a big ol' list of floats
		// representing x y z coordinates
		unsigned int numChildren = ndGetNumOfChildren( branch );
		if ( numChildren % 3 == 0 ) {
			unsigned int numVertices = numChildren / 3;
			if ( numVertices > 0 ) {
				float *vertices = PL_NEW_( float, numChildren );
				ndGetF32Array( branch, vertices, numChildren );
				for ( unsigned int i = 0, j = 0; i < numVertices; ++i, j += 3 ) {
					ApeWorldVertex *vertex = PL_NEW( ApeWorldVertex );
					vertex->position.x = vertices[ j ];
					vertex->position.y = vertices[ j + 1 ];
					vertex->position.z = vertices[ j + 2 ];
					PlPushBackVectorArrayElement( world->vertices, vertex );
				}

				PL_DELETE( vertices );
			}
		} else {
			PRINT_WARNING( "Invalid number of vertices for geometry!\n" );
		}
	}

	if ( ( branch = ndGetChildByName( root, "faces" ) ) != NULL ) {
		unsigned int numFaces = ndGetNumOfChildren( branch );
		if ( numFaces > 0 ) {
			branch = ndGetFirstChild( branch );
			while ( branch != NULL ) {
				DeserializeFace( world, root );
				branch = ndGetNextChild( branch );
			}
		}
	}
}

/****************************************
 * PUBLIC
 ****************************************/

ApeWorld *apeDeserializeWorld( ApeWorld *world, NdBranch *root ) {
	// Get the world version from the branch
	unsigned int version = ndGetUInt( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 ) {
		// Print a warning and return NULL if the version is not found
		PRINT_WARNING( "Failed to find world version!\n" );
		return NULL;
	} else if ( version > APE_WORLD_VERSION ) {
		// Print a warning and return NULL if the version is not supported
		PRINT_WARNING( "Unsupported world version! (%d > %d)\n", version, APE_WORLD_VERSION );
		return NULL;
	}

	// Get the properties branch from the root
	NdBranch *propertyList = ndGetChildByName( root, "properties" );
	if ( propertyList != NULL ) {
		// Copy the branch, so we can pass it over to the game logic later
		world->globalProperties = ndCopyBranch( propertyList );

		// Get the global properties of the world from the branch

		world->ambience = ndGetColourF32( world->globalProperties, "ambience", &PL_COLOURF32( 0.5f, 0.5f, 0.5f, 1.0f ) );
		world->sunColour = ndGetColourF32( world->globalProperties, "sunColour", &PL_COLOURF32_BLACK );
		world->sunPosition = ndGetVector3( world->globalProperties, "sunPosition", &pl_vecOrigin3 );

		world->clearColour = ndGetColourF32( world->globalProperties, "clearColour", &PL_COLOURF32_BLACK );

		world->fogColour = ndGetColourF32( world->globalProperties, "fogColour", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 0.0f ) );
		world->fogFar = ndGetF32ByName( world->globalProperties, "fogFar", 11.0f );
		world->fogNear = ndGetF32ByName( world->globalProperties, "fogNear", 32.0f );
	}

	// Deserialize the geometry and lights of the world from the root
	DeserializeGeometry( world, root );
	DeserializeLights( world, root );

	return world;
}
