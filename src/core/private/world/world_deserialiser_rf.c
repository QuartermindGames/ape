// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Deserialization methods specific to Volition's RFL format.

#include "ape_private.h"

#include "client/renderer/renderer.h"
#include "world.h"

#include "yin/core_fs.h"

//#define FLIP_WORLD// X coord needs to be flipped to match APE Tech coordinates...
#ifdef FLIP_WORLD
#	define FLIP_VECTOR( X ) ( ( X ).x *= -1 )
#else
#	define FLIP_VECTOR( X )
#endif

static const unsigned int RFL_MAGIC = 0xd4bada55;

static const int RFL_VERSION_MIN = 161;
static const int RFL_VERSION_MAX = 295;
// version history...
//	161	Red Faction (PS2 Prototype)
//	180	Red Faction (PC Demo)
//	272	Red Faction 2 (PS2 Demo)
// 	482	The Punisher (PS2)

// Below is a list of all the known used chunk types
#define RFL_CHUNK_GEOMETRY          0x100
#define RFL_CHUNK_GEOREGIONS        0x200
#define RFL_CHUNK_LIGHTS            0x300
#define RFL_CHUNK_CUTSCENECAMERAS   0x400
#define RFL_CHUNK_AMBIENT_SOUNDS    0x500
#define RFL_CHUNK_EVENTS            0x600
#define RFL_CHUNK_RESPAWN_POINTS    0x700
#define RFL_CHUNK_LEVEL_PROPERTIES  0x900
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
#define RFL_CHUNK_UNKNOWN_7000      0x7000
#define RFL_CHUNK_UNKNOWN_7001      0x7001
#define RFL_CHUNK_UNKNOWN_7002      0x7002
#define RFL_CHUNK_UNKNOWN_7003      0x7003
#define RFL_CHUNK_UNKNOWN_7004      0x7004
#define RFL_CHUNK_UNKNOWN_7005      0x7005
#define RFL_CHUNK_UNKNOWN_7678      0x7678
#define RFL_CHUNK_UNKNOWN_7680      0x7680
#define RFL_CHUNK_UNKNOWN_7681      0x7681
#define RFL_CHUNK_UNKNOWN_7900      0x7900
#define RFL_CHUNK_UNKNOWN_7901      0x7901
#define RFL_CHUNK_EAX               0x8000
#define RFL_CHUNK_WAYPOINTS         0x10000
#define RFL_CHUNK_NAVPOINTS         0x20000
#define RFL_CHUNK_ENTITIES          0x30000
#define RFL_CHUNK_ITEMS             0x40000
#define RFL_CHUNK_CLUTTER           0x50000
#define RFL_CHUNK_TRIGGERS          0x60000
#define RFL_CHUNK_PLAYER_START      0x70000

static void parse_static_geometry_textures( ApeWorld *world, PLFile *file )
{
	// fetch all the textures we'll be using
	uint32_t numTextures = PL_READUINT32( file, false, NULL );
	world->materials = PlCreateVectorArray( numTextures );
	for ( uint32_t i = 0; i < numTextures; ++i )
	{
		uint16_t size;
		char *textureName = acl_fs_parse_string( file, &size );
		assert( textureName != NULL );
		if ( textureName == NULL )
		{
			PRINT_WARNING( "Invalid texture (%u) name!\n", i );
			continue;
		}
		PRINT_DEBUG( "Texture: %s\n", textureName );

		char *c = strrchr( textureName, '.' );
		if ( c != NULL )
		{
			*c = '\0';
		}

		PLPath path;
		PlSetupPath( path, true, "materials/world/%s.mat.n", textureName );

		PlPushBackVectorArrayElement( world->materials, apeCacheMaterial( path, APE_CACHE_WORLD, true, false ) );

		PL_DELETE( textureName );
	}
}

static void parse_static_geometry_rooms( ApeWorld *world, PLFile *file, int32_t version )
{
	// fetch and populate the room list
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	world->rooms = PlCreateVectorArray( numRooms );
	for ( uint32_t i = 0; i < numRooms; ++i )
	{
		ApeWorldRoom *room = apeCreateWorldRoom();

		room->uid = PlReadInt32( file, false, NULL );

		room->bounds.mins = acl_fs_parse_vector( file );
		FLIP_VECTOR( room->bounds.mins );
		room->bounds.maxs = acl_fs_parse_vector( file );
		FLIP_VECTOR( room->bounds.maxs );

		if ( version >= 234 )
			room->flags = PL_READUINT32( file, false, NULL );
		else
		{
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_SKY; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_COLD; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_OUTSIDE; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_AIRLOCK; }
			room->containsLiquid = ( bool ) PL_READUINT8( file, NULL );
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_AMBIENT; }
			room->isDetail = ( bool ) PL_READUINT8( file, NULL );
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_ALPHA; }
		}

		room->life = acl_fs_parse_float( file );

		if ( version >= 180 )
		{
			uint16_t size;
			char *eaxEffect = acl_fs_parse_string( file, &size );
			if ( eaxEffect != NULL )
			{
				assert( isalpha( *eaxEffect ) );
				if ( isalpha( *eaxEffect ) )
					room->reverbPreset = get_eax_effect_id( eaxEffect );
				else
					PRINT_WARNING( "EAX effect is not a valid string! (%lu)", PlGetFileOffset( file ) );

				PL_DELETE( eaxEffect );
			}
		}

		if ( version >= 234 )
		{
			acl_fs_parse_float( file );
			acl_fs_parse_float( file );
			acl_fs_parse_float( file );

			PLColour colour = acl_fs_parse_colour( file );
			room->liquid.colour = PlColourU8ToF32( &colour );

			room->liquid.visibility = acl_fs_parse_float( file );

			room->liquid.type = PlReadInt32( file, false, NULL );

			if ( version < 284 )
			{
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = acl_fs_parse_float( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
			}

			room->liquid.panU = acl_fs_parse_float( file );
			room->liquid.panV = acl_fs_parse_float( file );

			if ( version >= 284 )
			{
				acl_fs_parse_float( file );
				acl_fs_parse_float( file );
			}

			if ( version < 284 )
			{
				acl_fs_parse_colour( file );
				PlReadInt32( file, false, NULL );

				if ( room->flags & APE_WORLD_ROOM_FLAG_UNKNOWN0 )
				{
					uint16_t size;
					char *tmp = acl_fs_parse_string( file, &size );
					PL_DELETE( tmp );
				}
			}
		}
		else
		{
			if ( room->containsLiquid )
			{
				room->liquid.depth = PlReadFloat32( file, false, NULL );
				assert( !isnan( room->liquid.depth ) );

				PLColour colour = acl_fs_parse_colour( file );
				room->liquid.colour = PlColourU8ToF32( &colour );

				uint16_t size;
				char *liquidTextureName = acl_fs_parse_string( file, &size );
				assert( liquidTextureName != NULL && *liquidTextureName != '\0' );
				PL_DELETE( liquidTextureName );

				room->liquid.visibility = acl_fs_parse_float( file );

				room->liquid.type = PlReadInt32( file, false, NULL );
				room->liquid.alpha = PlReadInt32( file, false, NULL );
				if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_PLANKTON; }
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = acl_fs_parse_float( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
				room->liquid.panU = acl_fs_parse_float( file );
				room->liquid.panV = acl_fs_parse_float( file );
			}

			if ( room->flags & APE_WORLD_ROOM_FLAG_AMBIENT )
			{
				PLColour colour = acl_fs_parse_colour( file );
				room->ambientLight = PlColourU8ToF32( &colour );
			}
		}

		PlPushBackVectorArrayElement( world->rooms, room );
	}
}

static void parse_static_geometry_detail_rooms( ApeWorld *world, PLFile *file )
{
	// something about sorting rooms into detail rooms list???
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	assert( numRooms == PlGetNumVectorArrayElements( world->rooms ) );
	for ( uint32_t i = 0; i < numRooms; ++i )
	{
		uint32_t roomIndex = PL_READUINT32( file, false, NULL );
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );

		uint32_t numDetailRooms = PL_READUINT32( file, false, NULL );
		if ( room != NULL && room->detailRooms == NULL )
		{
			room->detailRooms = PlCreateVectorArray( numDetailRooms );
		}

		for ( uint32_t j = 0; j < numDetailRooms; ++j )
		{
			uint32_t detailRoomIndex = PL_READUINT32( file, false, NULL );
			ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( world->rooms, detailRoomIndex );
			assert( detailRoom != NULL );
			if ( room != NULL && detailRoom != NULL )
			{
				detailRoom->isDetail = true;
				PlPushBackVectorArrayElement( room->detailRooms, detailRoom );
			}
		}
	}
}

static void parse_static_geometry_portals( ApeWorld *world, PLFile *file )
{
	uint32_t numPortals = PL_READUINT32( file, false, NULL );
	world->portals = PlCreateVectorArray( numPortals );
	for ( uint32_t i = 0; i < numPortals; ++i )
	{
		uint32_t roomAIndex = PL_READUINT32( file, false, NULL );
		uint32_t roomBIndex = PL_READUINT32( file, false, NULL );

		PLVector3 mins = acl_fs_parse_vector( file );
		FLIP_VECTOR( mins );
		PLVector3 maxs = acl_fs_parse_vector( file );
		FLIP_VECTOR( maxs );

		ApeWorldRoom *roomA = PlGetVectorArrayElementAt( world->rooms, roomAIndex );
		ApeWorldRoom *roomB = PlGetVectorArrayElementAt( world->rooms, roomBIndex );
		assert( roomA != NULL && roomB != NULL );
		if ( roomA == NULL || roomB == NULL )
		{
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

static void parse_static_geometry_vertices( ApeWorld *world, PLFile *file )
{
	uint32_t numVertices = PL_READUINT32( file, false, NULL );
	world->vertices = PlCreateVectorArray( numVertices );
	for ( uint32_t i = 0; i < numVertices; ++i )
	{
		ApeWorldVertex *vertex = PL_NEW( ApeWorldVertex );
		vertex->position = acl_fs_parse_vector( file );
		FLIP_VECTOR( vertex->position );
		PlPushBackVectorArrayElement( world->vertices, vertex );
	}
}

void apeGenerateWorldFaceBounds( ApeWorldFace *face )
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

static void calculate_face_normal( ApeWorldFace *face )
{
	unsigned int numVertices = PlGetNumVectorArrayElements( face->vertices );
	assert( numVertices > 0 );
	if ( numVertices == 0 )
	{
		return;
	}

	//TODO
	assert( 0 );
}

static void parse_static_geometry_faces( ApeWorld *world, PLFile *file, int32_t version )
{
	uint32_t numFaces = PL_READUINT32( file, false, NULL );

	for ( uint32_t i = 0; i < numFaces; ++i )
	{
		ApeWorldFace *face = PL_NEW( ApeWorldFace );

		face->edgeLoop = PlCreateLinkedList();

		if ( version >= 167 )
		{
			// plane
			face->normal = acl_fs_parse_vector( file );// normal
			FLIP_VECTOR( face->normal );
			face->offset = acl_fs_parse_float( file );// offset
		}

		face->materialIndex = PlReadInt32( file, false, NULL );
		if ( face->materialIndex >= 0 )
		{
			face->material = PlGetVectorArrayElementAt( world->materials, face->materialIndex );
			assert( face->material != NULL );
		}
		// some texture indices are negative, which is valid

		if ( face->material == NULL )
			face->material = ar_material_get_fallback();

		int32_t lightmapIndex = PlReadInt32( file, false, NULL );

		// ???
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );

		int32_t portalIndex = PlReadInt32( file, false, NULL );
		if ( portalIndex >= 0 )
		{
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
		for ( uint32_t j = 0; j < numFaceVertices; ++j )
		{
			ApeWorldFaceVertex *faceVertex = PL_NEW( ApeWorldFaceVertex );

			int32_t worldVertexIndex = PlReadInt32( file, false, NULL );
			assert( worldVertexIndex >= 0 );
			if ( worldVertexIndex >= 0 )
			{
				faceVertex->u = ( ApeWorldVertex * ) PlGetVectorArrayElementAt( world->vertices, worldVertexIndex );
				assert( faceVertex->u != NULL );
				if ( faceVertex->u == NULL )
					PRINT_ERROR( "Invalid world vertex for face!\n" );

				if ( faceVertex->u->adjacentFaces == NULL )
					faceVertex->u->adjacentFaces = PlCreateVectorArray( 1 );

				PlPushBackVectorArrayElement( faceVertex->u->adjacentFaces, face );
			}

			faceVertex->uv.x = acl_fs_parse_float( file );
			assert( faceVertex->uv.x * faceVertex->uv.x >= 0.0f );
			faceVertex->uv.y = acl_fs_parse_float( file );
			assert( faceVertex->uv.y * faceVertex->uv.y >= 0.0f );

			// Initially, we can just derive the face vertex normal from the face normal,
			// but these will need to be generated based on smoothing groups later
			faceVertex->normal = face->normal;

			if ( lightmapIndex >= 0 )
			{
				faceVertex->lightmapU = acl_fs_parse_float( file );
				faceVertex->lightmapV = acl_fs_parse_float( file );
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
		if ( version < 167 )
			calculate_face_normal( face );

		apeGenerateWorldFaceBounds( face );

		PlPushBackVectorArrayElement( room->faces, face );
	}
}

static void parse_static_geometry_lightmaps( ApeWorld *world, PLFile *file )
{
	int32_t numLightmaps = PlReadInt32( file, false, NULL );
	for ( int32_t i = 0; i < numLightmaps; ++i )
	{
		PlReadInt32( file, false, NULL );// lightmap index
		PL_READUINT8( file, NULL );      // x start
		PL_READUINT8( file, NULL );      // y start

		uint8_t width = PL_READUINT8( file, NULL );// width
		assert( width != 0 );
		uint8_t height = PL_READUINT8( file, NULL );// height
		assert( height != 0 );

		float xPerMeter = acl_fs_parse_float( file );// x pixels per meter
		float yPerMeter = acl_fs_parse_float( file );// y pixels per meter

		PLVector3 min = acl_fs_parse_vector( file );// min
		PLVector3 max = acl_fs_parse_vector( file );// max

		acl_fs_parse_vector( file );     // eq
		acl_fs_parse_float( file );      // offset
		PlReadInt32( file, false, NULL );// should smooth
		PlReadInt32( file, false, NULL );// fullbright
		PlReadInt32( file, false, NULL );// dropped coefficient
		PlReadInt32( file, false, NULL );// u coefficient
		PlReadInt32( file, false, NULL );// v coefficient
		acl_fs_parse_float( file );      // uv add x
		acl_fs_parse_float( file );      // uv add y
		acl_fs_parse_float( file );      // uv scale x
		acl_fs_parse_float( file );      // uv scale y

		int32_t roomIndex = PlReadInt32( file, false, NULL );// room index
		assert( PlGetVectorArrayElementAt( world->rooms, roomIndex ) != NULL );
	}
}

static void parse_static_geometry_texture_movers( ApeWorld *world, PLFile *file )
{
	// texture movers
	uint32_t numTextureMovers = PL_READUINT32( file, false, NULL );
	for ( uint32_t i = 0; i < numTextureMovers; ++i )
	{
		int32_t faceIndex = PlReadInt32( file, false, NULL );
		assert( faceIndex >= 0 );

		ApeWorldFace *face;
		if ( faceIndex >= 0 )
		{
			//face = ( ApeWorldFace * ) PlGetVectorArrayElementAt( world->faces, faceIndex );
			//assert( face != NULL );
		}

		float uPanSpeed = acl_fs_parse_float( file );
		float vPanSpeed = acl_fs_parse_float( file );
	}
}


static ApeWorld *parse_static_geometry_chunk( ApeWorld *world, PLFile *file, int32_t version )
{
	if ( version >= 200 )
	{
		PL_READUINT32( file, false, NULL );// version, unused
		PL_READUINT32( file, false, NULL );// "modifiability" - unused
	}

	// unused string
	uint16_t size = PL_READUINT16( file, false, NULL );
	PlFileSeek( file, size, PL_SEEK_CUR );

	if ( version < 200 )
		PL_READUINT32( file, false, NULL );// "modifiability" - unused

	parse_static_geometry_textures( world, file );

	if ( version >= 180 )
	{
		uint32_t numScrollingFaces = PL_READUINT32( file, false, NULL );
		for ( uint32_t i = 0; i < numScrollingFaces; ++i )
		{
			assert( 0 );
			// todo

			PlReadInt32( file, false, NULL );
			PlReadInt32( file, false, NULL );
			acl_fs_parse_float( file );//x
			acl_fs_parse_float( file );//y
			acl_fs_parse_float( file );//x
			acl_fs_parse_float( file );//y
			acl_fs_parse_float( file );//x
			acl_fs_parse_float( file );//y
			acl_fs_parse_float( file );//x
			acl_fs_parse_float( file );//y
			PlReadInt8( file, NULL );
		}
	}

	parse_static_geometry_rooms( world, file, version );
	parse_static_geometry_detail_rooms( world, file );
	parse_static_geometry_portals( world, file );
	parse_static_geometry_vertices( world, file );
	parse_static_geometry_faces( world, file, version );
	parse_static_geometry_lightmaps( world, file );
	//TODO: commented out for now, as it's causing issues with newer levels -
	// 		and I get the impression it was removed but too lazy to check,
	// 		plus we're not doing anything with it right now anyway...
	//ParseStaticGeometryTextureMovers( world, file );

	return NULL;
}

static void parse_lights_chunk( ApeWorld *world, PLFile *file, int32_t version )
{
	int32_t numLights = PlReadInt32( file, false, NULL );
	PlResizeVectorArray( world->lights, numLights );
	for ( int32_t i = 0; i < numLights; ++i )
	{
		ApeLight *light = PL_NEW( ApeLight );

		PlReadInt32( file, false, NULL );// id

		uint16_t size;
		char *tmp = acl_fs_parse_string( file, &size );// class name
		PL_DELETE( tmp );

		light->position = acl_fs_parse_vector( file );
		FLIP_VECTOR( light->position );

		acl_fs_parse_mat3( file );// rotation

		tmp = acl_fs_parse_string( file, &size );// script name
		PL_DELETE( tmp );

		light->isHidden = PL_READUINT8( file, NULL );     // hidden in editor
		light->flags = PL_READUINT32( file, false, NULL );// flags

		PLColour colour = acl_fs_parse_colour( file );
		light->colour = PlColourU8ToF32( &colour );

		light->radius = acl_fs_parse_float( file );// * 2.0f;

		acl_fs_parse_float( file );      // fov
		acl_fs_parse_float( file );      // fov dropoff
		acl_fs_parse_float( file );      // intensity at max range
		PlReadInt32( file, false, NULL );// dropoff type
		acl_fs_parse_float( file );      // tube light width
		acl_fs_parse_float( file );      // on intensity
		acl_fs_parse_float( file );      // on time
		acl_fs_parse_float( file );      // on time variation
		acl_fs_parse_float( file );      // off intensity
		acl_fs_parse_float( file );      // off time
		acl_fs_parse_float( file );      // off time variation

		PlPushBackVectorArrayElement( world->lights, light );
	}
}

static void parse_player_start( ApeWorld *world, PLFile *file, int32_t version )
{
	world->startPosition = acl_fs_parse_vector( file );
	FLIP_VECTOR( world->startPosition );
	world->startOrientation = acl_fs_parse_mat3( file );
}

static void parse_level_properties( ApeWorld *world, PLFile *file, int version )
{
	uint16_t size;
	char *texture = acl_fs_parse_string( file, &size );
	if ( texture != NULL )
		PL_DELETE( texture );

	int hardness = PlReadInt32( file, false, NULL );

	PLColour ambience = acl_fs_parse_colour( file );
	world->ambience = PlColourU8ToF32( &ambience );
	bool directionalAmbience = PL_READUINT8( file, NULL );

	PLColour fogColour = acl_fs_parse_colour( file );
	world->fogColour = PlColourU8ToF32( &fogColour );
	world->fogNear = acl_fs_parse_float( file );
	world->fogFar = acl_fs_parse_float( file );

	// Ensure clear copies the fog, to ensure some level of consistency
	world->clearColour = world->fogColour;
}

ApeWorld *apeParseRFWorld_( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	if ( magic != RFL_MAGIC )
	{
		PRINT_WARNING( "Invalid magic for world: %x != %x\n", magic, RFL_MAGIC );
		return NULL;
	}

	int32_t version = PlReadInt32( file, false, NULL );
	if ( version < RFL_VERSION_MIN || version > RFL_VERSION_MAX )
	{
		PRINT_WARNING( "Invalid version for world: %d < %d || %d > %d\n",
		               version, RFL_VERSION_MIN,
		               version, RFL_VERSION_MAX );
		return NULL;
	}

	uint32_t timestamp = PL_READUINT32( file, false, NULL );

	uint32_t objectOffset = PL_READUINT32( file, false, NULL );
	uint32_t editorOffset = PL_READUINT32( file, false, NULL );

	ApeWorld *level = ape_world_create();

	// read in all the chunks
	uint32_t numChunks = PL_READUINT32( file, false, NULL );
	uint32_t totalChunkSize = PL_READUINT32( file, false, NULL );
	if ( version > 161 )
	{
		uint16_t size;
		level->name = acl_fs_parse_string( file, &size );
	}
	if ( version >= 178 && version < 272 )
	{
		uint16_t size;
		char *modName = acl_fs_parse_string( file, &size );
		PL_DELETE( modName );
	}

	for ( uint32_t i = 0; i < numChunks; ++i )
	{
		uint32_t chunkId = PL_READUINT32( file, false, NULL );
		uint32_t chunkSize = PL_READUINT32( file, false, NULL );

		PLFileOffset offset = PlGetFileOffset( file );
		PLFileOffset nextChunk = offset + chunkSize;

		switch ( chunkId )
		{
			case RFL_CHUNK_GEOMETRY:
				parse_static_geometry_chunk( level, file, version );
				break;
			case RFL_CHUNK_LIGHTS:
				parse_lights_chunk( level, file, version );
				break;
			case RFL_CHUNK_PLAYER_START:
				parse_player_start( level, file, version );
				break;
			case RFL_CHUNK_LEVEL_PROPERTIES:
				parse_level_properties( level, file, version );
				break;
			default:
				PRINT_WARNING( "Skipping unknown chunk (%x : %u)\n", chunkId, offset );
				break;
		}

		// always do this afterwards, just on the off-chance the chunk failed to read
		PlFileSeek( file, nextChunk, PL_SEEK_SET );
	}

	return level;
}
