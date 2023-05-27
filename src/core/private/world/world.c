// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Loader for RFL format.

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "core_private.h"
#include "world.h"

#include <yin/node.h>

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_material.h"

void ogeSetupGlobalWorldDefaults( OgeWorld *world )
{
	world->ambience    = WORLD_DEFAULT_AMBIENCE;
	world->sunColour   = WORLD_DEFAULT_SUNCOLOUR;
	world->sunPosition = WORLD_DEFAULT_SUNPOSITION;
	world->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

OgeWorld *ogeCreateWorld( void )
{
	OgeWorld *world = PlMAllocA( sizeof( OgeWorld ) );

	world->globalProperties = ndPushBackObject( NULL, "properties" );
	ndPushBackF32Array( world->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	ndPushBackF32Array( world->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );
	//NL_PushBackStrArray( world->globalProperties, "skyMaterials", ( const char ** ) WORLD_DEFAULT_SKY, 1 );

	world->meshes   = PlCreateVectorArray( 0 );
	world->entities = PlCreateLinkedList();

	world->ambience = ( PLColourF32 ){ 1.0f, 1.0f, 1.0f, 1.0f };

	return world;
}

OgeWorld *YnCore_World_LoadFromNode( NdBranch *root )
{
	OgeWorld *world = ogeCreateWorld();
	if ( world != NULL && YnCore_WorldDeserialiser_Begin( root, world ) == NULL )
	{
		ogeDestroyWorld( world );
		world = NULL;
	}

	return world;
}

static const uint32_t WORLD_MAGIC = 0xd4bada55;

static const int32_t WORLD_VERSION_MIN = 161;
static const int32_t WORLD_VERSION_MAX = 180;

#define OGE_WORLD_CHUNK_GEOMETRY        0x100
#define OGE_WORLD_CHUNK_GEOREGIONS      0x200
#define OGE_WORLD_CHUNK_LIGHTS          0x300
#define OGE_WORLD_CHUNK_CUTSCENECAMERAS 0x400
#define OGE_WORLD_CHUNK_AMBIENTSOUNDS   0x500
#define OGE_WORLD_CHUNK_EVENTS          0x600
#define OGE_WORLD_CHUNK_EMITTERS        0xa00
#define OGE_WORLD_CHUNK_DECALS          0x1000
#define OGE_WORLD_CHUNK_LIGHTMAP        0x1200
#define OGE_WORLD_CHUNK_BRUSH           0x2000

static char *ParseString( PLFile *file, uint16_t *size )
{
	*size = PL_READUINT16( file, false, NULL );
	if ( *size == 0 )
	{
		return NULL;
	}

	char *buf = PL_NEW_( char, ( *size ) + 1 );
	PlReadFile( file, buf, sizeof( char ), *size );
	return buf;
}

static PLVector3 ParseVector( PLFile *file )
{
	return ( PLVector3 ){
	        PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ) };
}

static PLColour ParseColour( PLFile *file )
{
	return ( PLColour ){
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ) };
}

static void ParseStaticGeometryTextures( OgeWorld *world, PLFile *file )
{
	// fetch all the textures we'll be using
	uint32_t numTextures = PL_READUINT32( file, false, NULL );
	world->materials     = PlCreateVectorArray( numTextures );
	for ( uint32_t i = 0; i < numTextures; ++i )
	{
		uint16_t size;
		char *textureName = ParseString( file, &size );
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

		PlPushBackVectorArrayElement( world->materials, ogeCacheMaterial( path, YN_CORE_CACHE_GROUP_WORLD, true, false ) );

		PL_DELETE( textureName );
	}
}

static void ParseStaticGeometryRooms( OgeWorld *world, PLFile *file, int32_t version )
{
	// fetch and populate room list
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	world->rooms      = PlCreateVectorArray( numRooms );
	for ( uint32_t i = 0; i < numRooms; ++i )
	{
		OgeWorldRoom *room = ogeCreateWorldRoom();

		room->uid = PlReadInt32( file, false, NULL );

		room->bounds.mins = ParseVector( file );
		assert( !PlIsVectorNaN( ( float * ) &room->bounds.mins, PL_MVECNUM( room->bounds.mins ) ) );
		room->bounds.maxs = ParseVector( file );
		assert( !PlIsVectorNaN( ( float * ) &room->bounds.maxs, PL_MVECNUM( room->bounds.maxs ) ) );

		room->isSky               = ( bool ) PL_READUINT8( file, NULL );
		room->isCold              = ( bool ) PL_READUINT8( file, NULL );
		room->isOutside           = ( bool ) PL_READUINT8( file, NULL );
		room->isAirLock           = ( bool ) PL_READUINT8( file, NULL );
		room->containsLiquid      = ( bool ) PL_READUINT8( file, NULL );
		room->ambientLightDefined = ( bool ) PL_READUINT8( file, NULL );
		room->isDetail            = ( bool ) PL_READUINT8( file, NULL );
		room->hasAlpha            = ( bool ) PL_READUINT8( file, NULL );

		room->life = PlReadFloat32( file, false, NULL );
		if ( room->life <= 0.0f )
		{
			room->isInvincible = true;
		}

		if ( version >= 180 )
		{
			uint16_t size;
			char *eaxEffect = ParseString( file, &size );
			PL_DELETE( eaxEffect );
		}

		if ( room->containsLiquid )
		{
			room->liquid.depth = PlReadFloat32( file, false, NULL );
			assert( !isnan( room->liquid.depth ) );
			room->liquid.colour = ParseColour( file );

			uint16_t size;
			char *liquidTextureName = ParseString( file, &size );
			assert( liquidTextureName != NULL && *liquidTextureName != '\0' );
			PL_DELETE( liquidTextureName );

			room->liquid.visibility = PlReadFloat32( file, false, NULL );
			assert( !isnan( room->liquid.visibility ) );

			room->liquid.type     = PlReadInt32( file, false, NULL );
			room->liquid.alpha    = PlReadInt32( file, false, NULL );
			room->liquid.plankton = ( bool ) PL_READUINT8( file, NULL );
			room->liquid.ppmU     = PlReadInt32( file, false, NULL );
			room->liquid.ppmV     = PlReadInt32( file, false, NULL );

			room->liquid.angle = PlReadFloat32( file, false, NULL );
			assert( !isnan( room->liquid.angle ) );

			room->liquid.waveform = PlReadInt32( file, false, NULL );
			room->liquid.panU     = PlReadFloat32( file, false, NULL );
			room->liquid.panV     = PlReadFloat32( file, false, NULL );
		}

		if ( room->ambientLightDefined )
		{
			room->ambientLight = ParseColour( file );
		}

		PlPushBackVectorArrayElement( world->rooms, room );
	}
}

static void ParseStaticGeometryDetailRooms( OgeWorld *world, PLFile *file )
{
	// something about sorting rooms into detail rooms list???
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	assert( numRooms == PlGetNumVectorArrayElements( world->rooms ) );
	for ( uint32_t i = 0; i < numRooms; ++i )
	{
		uint32_t roomIndex = PL_READUINT32( file, false, NULL );
		OgeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );

		uint32_t numDetailRooms = PL_READUINT32( file, false, NULL );
		if ( room != NULL && room->detailRooms == NULL )
		{
			room->detailRooms = PlCreateVectorArray( numDetailRooms );
		}
		for ( uint32_t j = 0; j < numDetailRooms; ++j )
		{
			uint32_t detailRoomIndex = PL_READUINT32( file, false, NULL );
			OgeWorldRoom *detailRoom = PlGetVectorArrayElementAt( world->rooms, detailRoomIndex );
			assert( detailRoom != NULL );
			if ( room != NULL )
			{
				PlPushBackVectorArrayElement( room->detailRooms, detailRoom );
			}
		}
	}
}

static void ParseStaticGeometryPortals( OgeWorld *world, PLFile *file )
{
	uint32_t numPortals = PL_READUINT32( file, false, NULL );
	world->portals      = PlCreateVectorArray( numPortals );
	for ( uint32_t i = 0; i < numPortals; ++i )
	{
		uint32_t roomAIndex = PL_READUINT32( file, false, NULL );
		uint32_t roomBIndex = PL_READUINT32( file, false, NULL );

		PLVector3 mins = ParseVector( file );
		assert( !PlIsVectorNaN( ( float * ) &mins, PL_MVECNUM( mins ) ) );
		PLVector3 maxs = ParseVector( file );
		assert( !PlIsVectorNaN( ( float * ) &maxs, PL_MVECNUM( maxs ) ) );

		OgeWorldRoom *roomA = PlGetVectorArrayElementAt( world->rooms, roomAIndex );
		OgeWorldRoom *roomB = PlGetVectorArrayElementAt( world->rooms, roomBIndex );
		assert( roomA != NULL && roomB != NULL );
		if ( roomA == NULL || roomB == NULL )
		{
			PRINT_WARNING( "Invalid rooms for portals!\n" );
			continue;
		}

		OgeWorldPortal *portal = PL_NEW( OgeWorldPortal );
		portal->roomA          = roomA;
		portal->roomB          = roomB;
		portal->mins           = mins;
		portal->maxs           = maxs;

		PlPushBackVectorArrayElement( roomA->portals, portal );
		PlPushBackVectorArrayElement( roomB->portals, portal );
		PlPushBackVectorArrayElement( world->portals, portal );
	}
}

static void ParseStaticGeometryVertices( OgeWorld *world, PLFile *file )
{
	uint32_t numVertices = PL_READUINT32( file, false, NULL );
	world->vertices      = PlCreateVectorArray( numVertices );
	for ( uint32_t i = 0; i < numVertices; ++i )
	{
		OgeWorldVertex *vertex = PL_NEW( OgeWorldVertex );
		vertex->position       = ParseVector( file );
		assert( !PlIsVectorNaN( ( float * ) &vertex->position, PL_MVECNUM( vertex->position ) ) );
		PlPushBackVectorArrayElement( world->vertices, vertex );
	}
}

static void ParseStaticGeometryFaces( OgeWorld *world, PLFile *file, int32_t version )
{
	uint32_t numFaces = PL_READUINT32( file, false, NULL );
	world->faces      = PlCreateVectorArray( numFaces );
	for ( uint32_t i = 0; i < numFaces; ++i )
	{
		OgeWorldFace *face = PL_NEW( OgeWorldFace );
		face->edgeLoop     = PlCreateLinkedList();

		if ( version >= 180 )
		{
			// plane
			ParseVector( file );               // normal
			PlReadFloat32( file, false, NULL );// offset
		}

		int32_t textureIndex = PlReadInt32( file, false, NULL );
		if ( textureIndex >= 0 )
		{
			face->material = PlGetVectorArrayElementAt( world->materials, textureIndex );
			assert( face->material != NULL );
		}

		int32_t lightmapIndex = PlReadInt32( file, false, NULL );

		// ???
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );

		int32_t portalIndex = PlReadInt32( file, false, NULL );
#if 0// portal index crap is weird...
		if ( portalIndex >= 0 )
		{
			face->portal = PlGetVectorArrayElementAt( world->portals, portalIndex );
			assert( face->portal != NULL );
		}
#endif

		face->flags = PL_READUINT32( file, false, NULL );

		int32_t smoothingGroup = PlReadInt32( file, false, NULL );

		int32_t roomIndex = PlReadInt32( file, false, NULL );
		assert( roomIndex >= 0 );
		OgeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );

		uint32_t numFaceVertices = PL_READUINT32( file, false, NULL );
		face->vertices           = PlCreateVectorArray( numFaceVertices );
		for ( uint32_t j = 0; j < numFaceVertices; ++j )
		{
			OgeWorldFaceVertex *faceVertex = PL_NEW( OgeWorldFaceVertex );

			int32_t worldVertexIndex = PlReadInt32( file, false, NULL );
			assert( worldVertexIndex >= 0 );
			if ( worldVertexIndex >= 0 )
			{
				faceVertex->u = ( OgeWorldVertex * ) PlGetVectorArrayElementAt( world->vertices, worldVertexIndex );
				assert( faceVertex->u != NULL );
			}

			faceVertex->textureU = PlReadFloat32( file, false, NULL );
			assert( !isnan( faceVertex->textureU ) && ( faceVertex->textureU * faceVertex->textureU >= 0.0f ) );
			faceVertex->textureV = PlReadFloat32( file, false, NULL );
			assert( !isnan( faceVertex->textureV ) && ( faceVertex->textureV * faceVertex->textureV >= 0.0f ) );

			if ( lightmapIndex >= 0 )
			{
				faceVertex->lightmapU = PlReadFloat32( file, false, NULL );
				assert( !isnan( faceVertex->lightmapU ) );
				faceVertex->lightmapV = PlReadFloat32( file, false, NULL );
				assert( !isnan( faceVertex->lightmapV ) );
			}

			PlInsertLinkedListNode( face->edgeLoop, faceVertex );

			PlPushBackVectorArrayElement( face->vertices, faceVertex );
		}

		PlPushBackVectorArrayElement( room->faces, face );
	}
}

static void ParseStaticGeometryLightmaps( OgeWorld *world, PLFile *file )
{
	int32_t numLightmaps = PlReadInt32( file, false, NULL );
	for ( int32_t i = 0; i < numLightmaps; ++i )
	{
		PlReadInt32( file, false, NULL );           // lightmap index
		PL_READUINT8( file, NULL );                 // x start
		PL_READUINT8( file, NULL );                 // y start

		uint8_t width = PL_READUINT8( file, NULL ); // width
		assert( width != 0 );
		uint8_t height = PL_READUINT8( file, NULL );// height
		assert( height != 0 );

		float xPerMeter = PlReadFloat32( file, false, NULL );// x pixels per meter
		assert( !isnan( xPerMeter ) );
		float yPerMeter = PlReadFloat32( file, false, NULL );// y pixels per meter
		assert( !isnan( yPerMeter ) );

		PLVector3 min = ParseVector( file );// min
		assert( !PlIsVectorNaN( ( float * ) &min, PL_MVECNUM( min ) ) );
		PLVector3 max = ParseVector( file );// max
		assert( !PlIsVectorNaN( ( float * ) &max, PL_MVECNUM( max ) ) );

		ParseVector( file );                                 // eq
		PlReadFloat32( file, false, NULL );                  // offset
		PlReadInt32( file, false, NULL );                    // should smooth
		PlReadInt32( file, false, NULL );                    // fullbright
		PlReadInt32( file, false, NULL );                    // dropped coefficient
		PlReadInt32( file, false, NULL );                    // u coefficient
		PlReadInt32( file, false, NULL );                    // v coefficient
		PlReadFloat32( file, false, NULL );                  // uv add x
		PlReadFloat32( file, false, NULL );                  // uv add y
		PlReadFloat32( file, false, NULL );                  // uv scale x
		PlReadFloat32( file, false, NULL );                  // uv scale y

		int32_t roomIndex = PlReadInt32( file, false, NULL );// room index
		assert( PlGetVectorArrayElementAt( world->rooms, roomIndex ) != NULL );
	}
}

static void ParseStaticGeometryTextureMovers( OgeWorld *world, PLFile *file )
{
	// texture movers
	uint32_t numTextureMovers = PL_READUINT32( file, false, NULL );
	for ( uint32_t i = 0; i < numTextureMovers; ++i )
	{
		int32_t faceIndex = PlReadInt32( file, false, NULL );
		assert( faceIndex >= 0 );

		OgeWorldFace *face;
		if ( faceIndex >= 0 )
		{
			face = ( OgeWorldFace * ) PlGetVectorArrayElementAt( world->faces, faceIndex );
			assert( face != NULL );
		}

		float uPanSpeed = PlReadFloat32( file, false, NULL );
		assert( !isnan( uPanSpeed ) );
		float vPanSpeed = PlReadFloat32( file, false, NULL );
		assert( !isnan( vPanSpeed ) );
	}
}

static OgeWorld *ParseStaticGeometryChunk( OgeWorld *world, PLFile *file, int32_t version )
{
	uint16_t size;
	char *name = ParseString( file, &size );
	PL_DELETE( name );

	uint32_t modifiability = PL_READUINT32( file, false, NULL );

	ParseStaticGeometryTextures( world, file );

	uint32_t unkNum0 = PL_READUINT32( file, false, NULL );
	for ( uint32_t i = 0; i < unkNum0; ++i )
	{
		assert( 0 );
		// todo
	}

	ParseStaticGeometryRooms( world, file, version );
	ParseStaticGeometryDetailRooms( world, file );
	ParseStaticGeometryPortals( world, file );
	ParseStaticGeometryVertices( world, file );
	ParseStaticGeometryFaces( world, file, version );
	ParseStaticGeometryLightmaps( world, file );
	ParseStaticGeometryTextureMovers( world, file );

	return NULL;
}

static OgeWorld *ParseWorldFile( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	if ( magic != WORLD_MAGIC )
	{
		PRINT_WARNING( "Invalid magic for world: %x != %x\n", magic, WORLD_MAGIC );
		return NULL;
	}

	int32_t version = PlReadInt32( file, false, NULL );
	if ( version < WORLD_VERSION_MIN || version > WORLD_VERSION_MAX )
	{
		PRINT_WARNING( "Invalid version for world: %d < %d || %d > %d\n",
		               version, WORLD_VERSION_MIN,
		               version, WORLD_VERSION_MAX );
		return NULL;
	}

	uint32_t timestamp = PL_READUINT32( file, false, NULL );

	uint32_t objectOffset = PL_READUINT32( file, false, NULL );
	uint32_t editorOffset = PL_READUINT32( file, false, NULL );

	OgeWorld *world = ogeCreateWorld();

	// read in all the chunks
	uint32_t numChunks      = PL_READUINT32( file, false, NULL );
	uint32_t totalChunkSize = PL_READUINT32( file, false, NULL );
	if ( version > 161 )
	{
		uint16_t size;
		world->name = ParseString( file, &size );
	}
	if ( version >= 178 )
	{
		uint16_t size;
		char *modName = ParseString( file, &size );
		PL_DELETE( modName );
	}

	for ( uint32_t i = 0; i < numChunks; ++i )
	{
		uint32_t chunkId   = PL_READUINT32( file, false, NULL );
		uint32_t chunkSize = PL_READUINT32( file, false, NULL );

		PLFileOffset offset    = PlGetFileOffset( file );
		PLFileOffset nextChunk = offset + chunkSize;

		switch ( chunkId )
		{
			case OGE_WORLD_CHUNK_GEOMETRY:
				ParseStaticGeometryChunk( world, file, version );
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

OgeWorld *ogeLoadWorld( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to load world: %s\n", PlGetError() );
		return NULL;
	}

	OgeWorld *world = ParseWorldFile( file );

	PlCloseFile( file );

	return world;
}

bool ogeSaveWorld( OgeWorld *world, const char *path )
{
	world->lastSaveTime = time( NULL );

	NdBranch *root = ndPushBackObject( NULL, "world" );

	YnCore_WorldSerialiser_Begin( world, root );
	snprintf( world->path, sizeof( world->path ), "%s", path );

	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
	{
		PRINT_WARNING( "Failed to save world (%s): %s\n", path, ndGetErrorMessage() );
		return false;
	}

	return true;
}

void ogeDestroyWorld( OgeWorld *world )
{
	if ( world == NULL )
	{
		return;
	}

	assert( 0 );
}

void ogeWorld_SpawnEntities( OgeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entities );
	while ( node != NULL )
	{
		OgeWorldEntity *worldEntity = ( OgeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		YnCore_EntityManager_CreateEntityFromPrefab( worldEntity->entityTemplate->name );
		node = PlGetNextLinkedListNode( node );
	}
}

/****************************************
 * Global World Properties
 ****************************************/

NdBranch *ogeWorld_GetProperty( OgeWorld *world, const char *propertyName )
{
	if ( world->globalProperties == NULL )
	{
		return NULL;
	}

	return ndGetChildByName( world->globalProperties, propertyName );
}

/****************************************
 * SECTOR
 ****************************************/

OgeLight *YnCore_WorldSector_GetVisibleLights( OgeWorldRoom *sector, unsigned int *numLights )
{
	// TODO: for now we're just going to return this static list...
	static OgeLight lights[] = {
	        {
             .position = { 10.0f, 10.0f, 10.0f },
             .colour   = { 1.0f, 0.0f, 0.0f, 16.0f },
             .radius   = 16.0f,
	         },
	};

	*numLights = PL_ARRAY_ELEMENTS( lights );
	return lights;
}

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague lookup.
 */
OgeWorldRoom *YnCore_World_GetSectorByGlobalOrigin( OgeWorld *world, const PLVector3 *globalOrigin )
{
	return NULL;
}

OgeWorldFace **YnCore_WorldSector_GetMeshFaces( OgeWorldRoom *sector, uint32_t *numFaces )
{
	if ( sector->mesh == NULL )
	{
		*numFaces = 0;
		return NULL;
	}

	*numFaces            = PlGetNumLinkedListNodes( sector->mesh->faces );
	OgeWorldFace **faces = PL_NEW_( OgeWorldFace *, *numFaces );

	PLLinkedListNode *faceNode = PlGetFirstNode( sector->mesh->faces );
	for ( unsigned int i = 0; i < *numFaces; ++i )
	{
		faces[ i ] = ( OgeWorldFace * ) PlGetLinkedListNodeUserData( faceNode );
		faceNode   = PlGetNextLinkedListNode( faceNode );
	}

	return faces;
}

/**
 * This is a little bit silly, but we're considering mirrors as a valid portal too...
 */
bool YnCore_World_IsFacePortal( const OgeWorldFace *face )
{
	return ( ( face->flags & WORLD_FACE_FLAG_MIRROR ) || ( face->flags & WORLD_FACE_FLAG_PORTAL ) );
}
