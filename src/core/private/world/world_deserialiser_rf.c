// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Deserialization methods specific to Volition's RF format.

#include "ape_private.h"

#include "client/renderer/renderer.h"
#include "world.h"

//#define FLIP_WORLD// X coord needs to be flipped to match APE Tech coordinates...

static const unsigned int RFL_MAGIC = 0xd4bada55;

static const int RFL_VERSION_MIN = 161;
static const int RFL_VERSION_MAX = 295;
// version history...
//	161	Red Faction (PS2 Prototype)
//	180	Red Faction (PC Demo)
//	272	Red Faction 2 (PS2 Demo)
// 	482	The Punisher (PS2)

#define RFL_CHUNK_GEOMETRY          0x100
#define RFL_CHUNK_GEOREGIONS        0x200
#define RFL_CHUNK_LIGHTS            0x300
#define RFL_CHUNK_CUTSCENECAMERAS   0x400
#define RFL_CHUNK_AMBIENTSOUNDS     0x500
#define RFL_CHUNK_EVENTS            0x600
#define RFL_CHUNK_LEVELPROPERTIES   0x900
#define RFL_CHUNK_EMITTERS          0xa00
#define RFL_CHUNK_CLIMBREGIONS      0xd00
#define RFL_CHUNK_BOLTEMITTERS      0xe00
#define RFL_CHUNK_TARGETS           0xf00
#define RFL_CHUNK_DECALS            0x1000
#define RFL_CHUNK_PUSHREGIONS       0x1100
#define RFL_CHUNK_LIGHTMAP          0x1200
#define RFL_CHUNK_MOVERS            0x2000
#define RFL_CHUNK_MOVINGGROUP       0x3000
#define RFL_CHUNK_CUTSCENES         0x4000
#define RFL_CHUNK_CUTSCENEPATHNODES 0x5000
#define RFL_CHUNK_CUTSCENEPATHS     0x6000
#define RFL_CHUNK_EAX               0x8000
#define RFL_CHUNK_WAYPOINTS         0x10000
#define RFL_CHUNK_NAVPOINTS         0x20000
#define RFL_CHUNK_ENTITIES          0x30000
#define RFL_CHUNK_ITEMS             0x40000
#define RFL_CHUNK_CLUTTER           0x50000
#define RFL_CHUNK_TRIGGERS          0x60000
#define RFL_CHUNK_PLAYERSTART       0x70000

static char *ParseString( PLFile *file, uint16_t *size ) {
	*size = PL_READUINT16( file, false, NULL );
	if ( *size == 0 ) {
		return NULL;
	}

	char *buf = PL_NEW_( char, ( *size ) + 1 );
	PlReadFile( file, buf, sizeof( char ), *size );
	return buf;
}

static PLVector3 ParseVector( PLFile *file ) {
#if defined( FLIP_WORLD )
	PLVector3 v = ( PLVector3 ){
	        -PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ) };
#else
	PLVector3 v = ( PLVector3 ){
	        PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ) };
#endif
	assert( !PlIsVector3NaN( &v ) );
	return v;
}

static float ParseFloat( PLFile *file ) {
	float f = PlReadFloat32( file, false, NULL );
	assert( !isnan( f ) );
	return f;
}

static PLMatrix3 ParseMat3( PLFile *file ) {
	PLMatrix3 m = ( PLMatrix3 ){
	        // forward
	        .m[ 0 ] = PlReadFloat32( file, false, NULL ),
	        .m[ 1 ] = PlReadFloat32( file, false, NULL ),
	        .m[ 2 ] = PlReadFloat32( file, false, NULL ),
	        // right
	        .m[ 3 ] = PlReadFloat32( file, false, NULL ),
	        .m[ 4 ] = PlReadFloat32( file, false, NULL ),
	        .m[ 5 ] = PlReadFloat32( file, false, NULL ),
	        // up
	        .m[ 6 ] = PlReadFloat32( file, false, NULL ),
	        .m[ 7 ] = PlReadFloat32( file, false, NULL ),
	        .m[ 8 ] = PlReadFloat32( file, false, NULL ),
	};
	assert( !PlIsVectorNaN( m.m, 9 ) );
	return m;
}

static PLColour ParseColour( PLFile *file ) {
	return ( PLColour ){
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ) };
}

static void ParseStaticGeometryTextures( ApeWorld *world, PLFile *file ) {
	// fetch all the textures we'll be using
	uint32_t numTextures = PL_READUINT32( file, false, NULL );
	world->materials = PlCreateVectorArray( numTextures );
	for ( uint32_t i = 0; i < numTextures; ++i ) {
		uint16_t size;
		char *textureName = ParseString( file, &size );
		assert( textureName != NULL );
		if ( textureName == NULL ) {
			PRINT_WARNING( "Invalid texture (%u) name!\n", i );
			continue;
		}
		PRINT_DEBUG( "Texture: %s\n", textureName );

		char *c = strrchr( textureName, '.' );
		if ( c != NULL ) {
			*c = '\0';
		}

		PLPath path;
		PlSetupPath( path, true, "materials/world/%s.mat.n", textureName );

		PlPushBackVectorArrayElement( world->materials, apeCacheMaterial( path, APE_CACHE_WORLD, true, false ) );

		PL_DELETE( textureName );
	}
}

static void ParseStaticGeometryRooms( ApeWorld *world, PLFile *file, int32_t version ) {
	// fetch and populate the room list
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	world->rooms = PlCreateVectorArray( numRooms );
	for ( uint32_t i = 0; i < numRooms; ++i ) {
		ApeWorldRoom *room = apeCreateWorldRoom();

		room->uid = PlReadInt32( file, false, NULL );

		room->bounds.mins = ParseVector( file );
		room->bounds.maxs = ParseVector( file );

		if ( version >= 234 ) {
			room->flags = PL_READUINT32( file, false, NULL );
		} else {
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_SKY; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_COLD; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_OUTSIDE; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_AIRLOCK; }
			room->containsLiquid = ( bool ) PL_READUINT8( file, NULL );
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_AMBIENT; }
			room->isDetail = ( bool ) PL_READUINT8( file, NULL );
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_ALPHA; }
		}

		room->life = ParseFloat( file );

		if ( version >= 180 ) {
			uint16_t size;
			char *eaxEffect = ParseString( file, &size );
			PL_DELETE( eaxEffect );
		}

		if ( version >= 234 ) {
			ParseFloat( file );
			ParseFloat( file );
			ParseFloat( file );

			PLColour colour = ParseColour( file );
			room->liquid.colour = PlColourU8ToF32( &colour );

			room->liquid.visibility = ParseFloat( file );

			room->liquid.type = PlReadInt32( file, false, NULL );

			if ( version < 284 ) {
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = ParseFloat( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
			}

			room->liquid.panU = ParseFloat( file );
			room->liquid.panV = ParseFloat( file );

			if ( version >= 284 ) {
				ParseFloat( file );
				ParseFloat( file );
			}

			if ( version < 284 ) {
				ParseColour( file );
				PlReadInt32( file, false, NULL );

				if ( room->flags & APE_WORLD_ROOM_FLAG_UNKNOWN0 ) {
					uint16_t size;
					char *tmp = ParseString( file, &size );
					PL_DELETE( tmp );
				}
			}
		} else {
			if ( room->containsLiquid ) {
				room->liquid.depth = PlReadFloat32( file, false, NULL );
				assert( !isnan( room->liquid.depth ) );

				PLColour colour = ParseColour( file );
				room->liquid.colour = PlColourU8ToF32( &colour );

				uint16_t size;
				char *liquidTextureName = ParseString( file, &size );
				assert( liquidTextureName != NULL && *liquidTextureName != '\0' );
				PL_DELETE( liquidTextureName );

				room->liquid.visibility = ParseFloat( file );

				room->liquid.type = PlReadInt32( file, false, NULL );
				room->liquid.alpha = PlReadInt32( file, false, NULL );
				if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_PLANKTON; }
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = ParseFloat( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
				room->liquid.panU = ParseFloat( file );
				room->liquid.panV = ParseFloat( file );
			}

			if ( room->flags & APE_WORLD_ROOM_FLAG_AMBIENT ) {
				PLColour colour = ParseColour( file );
				room->ambientLight = PlColourU8ToF32( &colour );
			}
		}

		PlPushBackVectorArrayElement( world->rooms, room );
	}
}

static void ParseStaticGeometryDetailRooms( ApeWorld *world, PLFile *file ) {
	// something about sorting rooms into detail rooms list???
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	assert( numRooms == PlGetNumVectorArrayElements( world->rooms ) );
	for ( uint32_t i = 0; i < numRooms; ++i ) {
		uint32_t roomIndex = PL_READUINT32( file, false, NULL );
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );

		uint32_t numDetailRooms = PL_READUINT32( file, false, NULL );
		if ( room != NULL && room->detailRooms == NULL ) {
			room->detailRooms = PlCreateVectorArray( numDetailRooms );
		}

		for ( uint32_t j = 0; j < numDetailRooms; ++j ) {
			uint32_t detailRoomIndex = PL_READUINT32( file, false, NULL );
			ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( world->rooms, detailRoomIndex );
			assert( detailRoom != NULL );
			if ( room != NULL && detailRoom != NULL ) {
				detailRoom->isDetail = true;
				PlPushBackVectorArrayElement( room->detailRooms, detailRoom );
			}
		}
	}
}

static void ParseStaticGeometryPortals( ApeWorld *world, PLFile *file ) {
	uint32_t numPortals = PL_READUINT32( file, false, NULL );
	world->portals = PlCreateVectorArray( numPortals );
	for ( uint32_t i = 0; i < numPortals; ++i ) {
		uint32_t roomAIndex = PL_READUINT32( file, false, NULL );
		uint32_t roomBIndex = PL_READUINT32( file, false, NULL );

		PLVector3 mins = ParseVector( file );
		PLVector3 maxs = ParseVector( file );

		ApeWorldRoom *roomA = PlGetVectorArrayElementAt( world->rooms, roomAIndex );
		ApeWorldRoom *roomB = PlGetVectorArrayElementAt( world->rooms, roomBIndex );
		assert( roomA != NULL && roomB != NULL );
		if ( roomA == NULL || roomB == NULL ) {
			PRINT_WARNING( "Invalid rooms for portals!\n" );
			continue;
		}

		ApeWorldPortal *portal = PL_NEW( ApeWorldPortal );
		portal->roomA = roomA;
		portal->roomB = roomB;
		portal->mins = mins;
		portal->maxs = maxs;

		PlPushBackVectorArrayElement( roomA->portals, portal );
		PlPushBackVectorArrayElement( roomB->portals, portal );
		PlPushBackVectorArrayElement( world->portals, portal );
	}
}

static void ParseStaticGeometryVertices( ApeWorld *world, PLFile *file ) {
	uint32_t numVertices = PL_READUINT32( file, false, NULL );
	world->vertices = PlCreateVectorArray( numVertices );
	for ( uint32_t i = 0; i < numVertices; ++i ) {
		ApeWorldVertex *vertex = PL_NEW( ApeWorldVertex );
		vertex->position = ParseVector( file );
		PlPushBackVectorArrayElement( world->vertices, vertex );
	}
}

void apeGenerateWorldFaceBounds( ApeWorldFace *face ) {
	unsigned int numVertices = PlGetNumVectorArrayElements( face->vertices );
	ApeWorldFaceVertex **vertices = ( ApeWorldFaceVertex ** ) PlGetVectorArrayData( face->vertices );
	if ( numVertices == 0 ) {
		return;
	}

	PLVector3 *boundVertices = PL_NEW_( PLVector3, numVertices );
	for ( unsigned int i = 0; i < numVertices; ++i ) {
		boundVertices[ i ] = vertices[ i ]->u->position;
	}

	face->bounds = PlGenerateAabbFromCoords( boundVertices, numVertices, true );
	face->origin = PlGetAabbAbsOrigin( &face->bounds, pl_vecOrigin3 );

	PL_DELETE( boundVertices );
}

static void CalculateFaceNormal( ApeWorldFace *face ) {
	unsigned int numVertices = PlGetNumVectorArrayElements( face->vertices );
	assert( numVertices > 0 );
	if ( numVertices == 0 ) {
		return;
	}

	//TODO
	assert( 0 );
}

static void ParseStaticGeometryFaces( ApeWorld *world, PLFile *file, int32_t version ) {
	uint32_t numFaces = PL_READUINT32( file, false, NULL );

	for ( uint32_t i = 0; i < numFaces; ++i ) {
		ApeWorldFace *face = PL_NEW( ApeWorldFace );

		face->edgeLoop = PlCreateLinkedList();

		if ( version >= 167 ) {
			// plane
			face->normal = ParseVector( file );// normal
			face->offset = ParseFloat( file ); // offset
		}

		face->materialIndex = PlReadInt32( file, false, NULL );
		if ( face->materialIndex >= 0 ) {
			face->material = PlGetVectorArrayElementAt( world->materials, face->materialIndex );
			assert( face->material != NULL );
		}
		// some texture indices are negative, which is valid

		if ( face->material == NULL ) {
			face->material = apeGetFallbackMaterial();
		}

		int32_t lightmapIndex = PlReadInt32( file, false, NULL );

		// ???
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );

		int32_t portalIndex = PlReadInt32( file, false, NULL );
		if ( portalIndex >= 0 ) {
			face->portal = PlGetVectorArrayElementAt( world->portals, portalIndex );
			//assert( face->portal != NULL );
		}

		face->flags = PL_READUINT32( file, false, NULL );

		face->smoothingGroup = PlReadInt32( file, false, NULL );

		int32_t roomIndex = PlReadInt32( file, false, NULL );
		assert( roomIndex >= 0 );
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );

		uint32_t numFaceVertices = PL_READUINT32( file, false, NULL );
		face->vertices = PlCreateVectorArray( numFaceVertices );
		for ( uint32_t j = 0; j < numFaceVertices; ++j ) {
			ApeWorldFaceVertex *faceVertex = PL_NEW( ApeWorldFaceVertex );

			int32_t worldVertexIndex = PlReadInt32( file, false, NULL );
			assert( worldVertexIndex >= 0 );
			if ( worldVertexIndex >= 0 ) {
				faceVertex->u = ( ApeWorldVertex * ) PlGetVectorArrayElementAt( world->vertices, worldVertexIndex );
				assert( faceVertex->u != NULL );
				if ( faceVertex->u == NULL ) {
					PRINT_ERROR( "Invalid world vertex for face!\n" );
				}

				if ( faceVertex->u->adjacentFaces == NULL ) {
					faceVertex->u->adjacentFaces = PlCreateVectorArray( 1 );
				}

				PlPushBackVectorArrayElement( faceVertex->u->adjacentFaces, face );
			}

			faceVertex->uv.x = ParseFloat( file );
			assert( faceVertex->uv.x * faceVertex->uv.x >= 0.0f );
			faceVertex->uv.y = ParseFloat( file );
			assert( faceVertex->uv.y * faceVertex->uv.y >= 0.0f );

			if ( lightmapIndex >= 0 ) {
				faceVertex->lightmapU = ParseFloat( file );
				faceVertex->lightmapV = ParseFloat( file );
			}

#if defined( FLIP_WORLD )
			PlInsertFrontLinkedListNode( face->edgeLoop, faceVertex );
#else
			PlInsertLinkedListNode( face->edgeLoop, faceVertex );
#endif

			PlPushBackVectorArrayElement( face->vertices, faceVertex );
		}

		// Older versions didn't store the face normal / offset,
		// so we'll need to calculate that here
		if ( version < 167 ) {
			CalculateFaceNormal( face );
		}

		apeGenerateWorldFaceBounds( face );

		PlPushBackVectorArrayElement( room->faces, face );
	}
}

static void ParseStaticGeometryLightmaps( ApeWorld *world, PLFile *file ) {
	int32_t numLightmaps = PlReadInt32( file, false, NULL );
	for ( int32_t i = 0; i < numLightmaps; ++i ) {
		PlReadInt32( file, false, NULL );// lightmap index
		PL_READUINT8( file, NULL );      // x start
		PL_READUINT8( file, NULL );      // y start

		uint8_t width = PL_READUINT8( file, NULL );// width
		assert( width != 0 );
		uint8_t height = PL_READUINT8( file, NULL );// height
		assert( height != 0 );

		float xPerMeter = ParseFloat( file );// x pixels per meter
		float yPerMeter = ParseFloat( file );// y pixels per meter

		PLVector3 min = ParseVector( file );// min
		PLVector3 max = ParseVector( file );// max

		ParseVector( file );             // eq
		ParseFloat( file );              // offset
		PlReadInt32( file, false, NULL );// should smooth
		PlReadInt32( file, false, NULL );// fullbright
		PlReadInt32( file, false, NULL );// dropped coefficient
		PlReadInt32( file, false, NULL );// u coefficient
		PlReadInt32( file, false, NULL );// v coefficient
		ParseFloat( file );              // uv add x
		ParseFloat( file );              // uv add y
		ParseFloat( file );              // uv scale x
		ParseFloat( file );              // uv scale y

		int32_t roomIndex = PlReadInt32( file, false, NULL );// room index
		assert( PlGetVectorArrayElementAt( world->rooms, roomIndex ) != NULL );
	}
}

static void ParseStaticGeometryTextureMovers( ApeWorld *world, PLFile *file ) {
	// texture movers
	uint32_t numTextureMovers = PL_READUINT32( file, false, NULL );
	for ( uint32_t i = 0; i < numTextureMovers; ++i ) {
		int32_t faceIndex = PlReadInt32( file, false, NULL );
		assert( faceIndex >= 0 );

		ApeWorldFace *face;
		if ( faceIndex >= 0 ) {
			//face = ( ApeWorldFace * ) PlGetVectorArrayElementAt( world->faces, faceIndex );
			//assert( face != NULL );
		}

		float uPanSpeed = ParseFloat( file );
		float vPanSpeed = ParseFloat( file );
	}
}


static ApeWorld *ParseStaticGeometryChunk( ApeWorld *world, PLFile *file, int32_t version ) {
	if ( version >= 200 ) {
		PL_READUINT32( file, false, NULL );// version, unused
	}
	if ( version >= 200 ) {
		PL_READUINT32( file, false, NULL );// "modifiability" - unused
	}

	// unused string
	uint16_t size = PL_READUINT16( file, false, NULL );
	PlFileSeek( file, size, PL_SEEK_CUR );

	if ( version < 200 ) {
		PL_READUINT32( file, false, NULL );// "modifiability" - unused
	}

	ParseStaticGeometryTextures( world, file );

	if ( version >= 180 ) {
		uint32_t numScrollingFaces = PL_READUINT32( file, false, NULL );
		for ( uint32_t i = 0; i < numScrollingFaces; ++i ) {
			assert( 0 );
			// todo

			PlReadInt32( file, false, NULL );
			PlReadInt32( file, false, NULL );
			ParseFloat( file );//x
			ParseFloat( file );//y
			ParseFloat( file );//x
			ParseFloat( file );//y
			ParseFloat( file );//x
			ParseFloat( file );//y
			ParseFloat( file );//x
			ParseFloat( file );//y
			PlReadInt8( file, NULL );
		}
	}

	ParseStaticGeometryRooms( world, file, version );
	ParseStaticGeometryDetailRooms( world, file );
	ParseStaticGeometryPortals( world, file );
	ParseStaticGeometryVertices( world, file );
	ParseStaticGeometryFaces( world, file, version );
	ParseStaticGeometryLightmaps( world, file );
	//TODO: commented out for now, as it's causing issues with newer levels -
	// 		and I get the impression it was removed but too lazy to check,
	// 		plus we're not doing anything with it right now anyway...
	//ParseStaticGeometryTextureMovers( world, file );

	return NULL;
}

static void ParseLightsChunk( ApeWorld *world, PLFile *file, int32_t version ) {
	int32_t numLights = PlReadInt32( file, false, NULL );
	PlResizeVectorArray( world->lights, numLights );
	for ( int32_t i = 0; i < numLights; ++i ) {
		ApeLight *light = PL_NEW( ApeLight );

		PlReadInt32( file, false, NULL );// id

		uint16_t size;
		char *tmp = ParseString( file, &size );// class name
		PL_DELETE( tmp );

		light->position = ParseVector( file );

		ParseMat3( file );// rotation

		tmp = ParseString( file, &size );// script name
		PL_DELETE( tmp );

		light->isHidden = PL_READUINT8( file, NULL );     // hidden in editor
		light->flags = PL_READUINT32( file, false, NULL );// flags

		PLColour colour = ParseColour( file );
		light->colour = PlColourU8ToF32( &colour );

		light->radius = ParseFloat( file );// * 2.0f;

		ParseFloat( file );              // fov
		ParseFloat( file );              // fov dropoff
		ParseFloat( file );              // intensity at max range
		PlReadInt32( file, false, NULL );// dropoff type
		ParseFloat( file );              // tube light width
		ParseFloat( file );              // on intensity
		ParseFloat( file );              // on time
		ParseFloat( file );              // on time variation
		ParseFloat( file );              // off intensity
		ParseFloat( file );              // off time
		ParseFloat( file );              // off time variation

		PlPushBackVectorArrayElement( world->lights, light );
	}
}

static void ParsePlayerStart( ApeWorld *world, PLFile *file, int32_t version ) {
	world->startPosition = ParseVector( file );
	world->startOrientation = ParseMat3( file );
}

static void ParseLevelProperties( ApeWorld *world, PLFile *file, int version ) {
	uint16_t size;
	char *texture = ParseString( file, &size );
	if ( texture != NULL ) {
		PL_DELETE( texture );
	}

	int hardness = PlReadInt32( file, false, NULL );

	PLColour ambience = ParseColour( file );
	world->ambience = PlColourU8ToF32( &ambience );
	bool directionalAmbience = PL_READUINT8( file, NULL );

	PLColour fogColour = ParseColour( file );
	world->fogColour = PlColourU8ToF32( &fogColour );
	world->fogNear = ParseFloat( file );
	world->fogFar = ParseFloat( file );

	// Ensure clear copies the fog, to ensure some level of consistency
	world->clearColour = world->fogColour;
}

ApeWorld *apeParseRFWorld_( PLFile *file ) {
	uint32_t magic = PL_READUINT32( file, false, NULL );
	if ( magic != RFL_MAGIC ) {
		PRINT_WARNING( "Invalid magic for world: %x != %x\n", magic, RFL_MAGIC );
		return NULL;
	}

	int32_t version = PlReadInt32( file, false, NULL );
	if ( version < RFL_VERSION_MIN || version > RFL_VERSION_MAX ) {
		PRINT_WARNING( "Invalid version for world: %d < %d || %d > %d\n",
		               version, RFL_VERSION_MIN,
		               version, RFL_VERSION_MAX );
		return NULL;
	}

	uint32_t timestamp = PL_READUINT32( file, false, NULL );

	uint32_t objectOffset = PL_READUINT32( file, false, NULL );
	uint32_t editorOffset = PL_READUINT32( file, false, NULL );

	ApeWorld *world = apeCreateWorld();

	// read in all the chunks
	uint32_t numChunks = PL_READUINT32( file, false, NULL );
	uint32_t totalChunkSize = PL_READUINT32( file, false, NULL );
	if ( version > 161 ) {
		uint16_t size;
		world->name = ParseString( file, &size );
	}
	if ( version >= 178 && version < 272 ) {
		uint16_t size;
		char *modName = ParseString( file, &size );
		PL_DELETE( modName );
	}

	for ( uint32_t i = 0; i < numChunks; ++i ) {
		uint32_t chunkId = PL_READUINT32( file, false, NULL );
		uint32_t chunkSize = PL_READUINT32( file, false, NULL );

		PLFileOffset offset = PlGetFileOffset( file );
		PLFileOffset nextChunk = offset + chunkSize;

		switch ( chunkId ) {
			case RFL_CHUNK_GEOMETRY:
				ParseStaticGeometryChunk( world, file, version );
				break;
			case RFL_CHUNK_LIGHTS:
				ParseLightsChunk( world, file, version );
				break;
			case RFL_CHUNK_PLAYERSTART:
				ParsePlayerStart( world, file, version );
				break;
			case RFL_CHUNK_LEVELPROPERTIES:
				ParseLevelProperties( world, file, version );
				break;
			default:
				PRINT_DEBUG( "Skipping unknown chunk (%x : %u)\n", chunkId, offset );
				break;
		}

		// always do this afterwards, just on the off-chance the chunk failed to read
		PlFileSeek( file, nextChunk, PL_SEEK_SET );
	}

	return world;
}
