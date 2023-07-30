// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Loader for RFL format.

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "ape_private.h"
#include "world.h"

#include <float.h>

#include "client/renderer/renderer.h"

void apeSetupGlobalWorldDefaults( ApeWorld *world )
{
	world->ambience    = WORLD_DEFAULT_AMBIENCE;
	world->sunColour   = WORLD_DEFAULT_SUNCOLOUR;
	world->sunPosition = WORLD_DEFAULT_SUNPOSITION;
	world->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

ApeWorld *apeCreateWorld( void )
{
	ApeWorld *world = PlMAllocA( sizeof( ApeWorld ) );

	world->globalProperties = ndPushBackObject( NULL, "properties" );
	ndPushBackF32Array( world->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	ndPushBackF32Array( world->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );

	world->meshes   = PlCreateVectorArray( 0 );
	world->entities = PlCreateLinkedList();

	apeInitializeWorldVisibilitySystem_();

	apeSetupGlobalWorldDefaults( world );

	return world;
}

static const uint32_t WORLD_MAGIC = 0xd4bada55;

static const int32_t WORLD_VERSION_MIN = 161;
static const int32_t WORLD_VERSION_MAX = 295;
// version history...
//	161	Red Faction (PS2 Prototype)
//	180	Red Faction (PC Demo)
//	272	Red Faction 2 (PS2 Demo)
// 	482	The Punisher (PS2)

#define APE_WORLD_CHUNK_GEOMETRY          0x100
#define APE_WORLD_CHUNK_GEOREGIONS        0x200
#define APE_WORLD_CHUNK_LIGHTS            0x300
#define APE_WORLD_CHUNK_CUTSCENECAMERAS   0x400
#define APE_WORLD_CHUNK_AMBIENTSOUNDS     0x500
#define APE_WORLD_CHUNK_EVENTS            0x600
#define APE_WORLD_CHUNK_UNKNOWN0          0x900
#define APE_WORLD_CHUNK_EMITTERS          0xa00
#define APE_WORLD_CHUNK_CLIMBREGIONS      0xd00
#define APE_WORLD_CHUNK_BOLTEMITTERS      0xe00
#define APE_WORLD_CHUNK_TARGETS           0xf00
#define APE_WORLD_CHUNK_DECALS            0x1000
#define APE_WORLD_CHUNK_PUSHREGIONS       0x1100
#define APE_WORLD_CHUNK_LIGHTMAP          0x1200
#define APE_WORLD_CHUNK_BRUSH             0x2000
#define APE_WORLD_CHUNK_MOVINGGROUP       0x3000
#define APE_WORLD_CHUNK_CUTSCENES         0x4000
#define APE_WORLD_CHUNK_CUTSCENEPATHNODES 0x5000
#define APE_WORLD_CHUNK_CUTSCENEPATHS     0x6000
#define APE_WORLD_CHUNK_WAYPOINTS         0x10000
#define APE_WORLD_CHUNK_NAVPOINTS         0x20000
#define APE_WORLD_CHUNK_ENTITIES          0x30000
#define APE_WORLD_CHUNK_ITEMS             0x40000
#define APE_WORLD_CHUNK_CLUTTER           0x50000
#define APE_WORLD_CHUNK_TRIGGERS          0x60000
#define APE_WORLD_CHUNK_PLAYERSTART       0x70000

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
	PLVector3 v = ( PLVector3 ){
	        PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ),
	        PlReadFloat32( file, false, NULL ) };
	assert( !PlIsVector3NaN( &v ) );
	return v;
}

static float ParseFloat( PLFile *file )
{
	float f = PlReadFloat32( file, false, NULL );
	assert( !isnan( f ) );
	return f;
}

static PLMatrix3 ParseMat3( PLFile *file )
{
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

static PLColour ParseColour( PLFile *file )
{
	return ( PLColour ){
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ),
	        PL_READUINT8( file, NULL ) };
}

static void ParseStaticGeometryTextures( ApeWorld *world, PLFile *file )
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

		PlPushBackVectorArrayElement( world->materials, apeCacheMaterial( path, APE_CACHE_WORLD, true, false ) );

		PL_DELETE( textureName );
	}
}

static void ParseStaticGeometryRooms( ApeWorld *world, PLFile *file, int32_t version )
{
	// fetch and populate the room list
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	world->rooms      = PlCreateVectorArray( numRooms );
	for ( uint32_t i = 0; i < numRooms; ++i )
	{
		ApeWorldRoom *room = apeCreateWorldRoom();

		room->uid = PlReadInt32( file, false, NULL );

		room->bounds.mins = ParseVector( file );
		room->bounds.maxs = ParseVector( file );

		if ( version >= 234 )
		{
			room->flags = PL_READUINT32( file, false, NULL );
		}
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

		room->life = ParseFloat( file );

		if ( version >= 180 )
		{
			uint16_t size;
			char *eaxEffect = ParseString( file, &size );
			PL_DELETE( eaxEffect );
		}

		if ( version >= 234 )
		{
			float x = ParseFloat( file );
			float y = ParseFloat( file );
			float z = ParseFloat( file );

			room->liquid.colour = ParseColour( file );

			room->liquid.visibility = ParseFloat( file );

			room->liquid.type = PlReadInt32( file, false, NULL );

			if ( version < 284 )
			{
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = ParseFloat( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
			}

			room->liquid.panU = ParseFloat( file );
			room->liquid.panV = ParseFloat( file );

			if ( version >= 284 )
			{
				ParseFloat( file );
				ParseFloat( file );
			}

			if ( version < 284 )
			{
				ParseColour( file );
				PlReadInt32( file, false, NULL );

				if ( room->flags & APE_WORLD_ROOM_FLAG_UNKNOWN0 )
				{
					uint16_t size;
					char *tmp = ParseString( file, &size );
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
				room->liquid.colour = ParseColour( file );

				uint16_t size;
				char *liquidTextureName = ParseString( file, &size );
				assert( liquidTextureName != NULL && *liquidTextureName != '\0' );
				PL_DELETE( liquidTextureName );

				room->liquid.visibility = ParseFloat( file );

				room->liquid.type  = PlReadInt32( file, false, NULL );
				room->liquid.alpha = PlReadInt32( file, false, NULL );
				if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_PLANKTON; }
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = ParseFloat( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
				room->liquid.panU     = ParseFloat( file );
				room->liquid.panV     = ParseFloat( file );
			}

			if ( room->flags & APE_WORLD_ROOM_FLAG_AMBIENT )
			{
				room->ambientLight = ParseColour( file );
			}
		}

		PlPushBackVectorArrayElement( world->rooms, room );
	}
}

static void ParseStaticGeometryDetailRooms( ApeWorld *world, PLFile *file )
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

static void ParseStaticGeometryPortals( ApeWorld *world, PLFile *file )
{
	uint32_t numPortals = PL_READUINT32( file, false, NULL );
	world->portals      = PlCreateVectorArray( numPortals );
	for ( uint32_t i = 0; i < numPortals; ++i )
	{
		uint32_t roomAIndex = PL_READUINT32( file, false, NULL );
		uint32_t roomBIndex = PL_READUINT32( file, false, NULL );

		PLVector3 mins = ParseVector( file );
		PLVector3 maxs = ParseVector( file );

		ApeWorldRoom *roomA = PlGetVectorArrayElementAt( world->rooms, roomAIndex );
		ApeWorldRoom *roomB = PlGetVectorArrayElementAt( world->rooms, roomBIndex );
		assert( roomA != NULL && roomB != NULL );
		if ( roomA == NULL || roomB == NULL )
		{
			PRINT_WARNING( "Invalid rooms for portals!\n" );
			continue;
		}

		ApeWorldPortal *portal = PL_NEW( ApeWorldPortal );
		portal->roomA          = roomA;
		portal->roomB          = roomB;
		portal->mins           = mins;
		portal->maxs           = maxs;

		PlPushBackVectorArrayElement( roomA->portals, portal );
		PlPushBackVectorArrayElement( roomB->portals, portal );
		PlPushBackVectorArrayElement( world->portals, portal );
	}
}

static void ParseStaticGeometryVertices( ApeWorld *world, PLFile *file )
{
	uint32_t numVertices = PL_READUINT32( file, false, NULL );
	world->vertices      = PlCreateVectorArray( numVertices );
	for ( uint32_t i = 0; i < numVertices; ++i )
	{
		ApeWorldVertex *vertex = PL_NEW( ApeWorldVertex );
		vertex->position       = ParseVector( file );
		PlPushBackVectorArrayElement( world->vertices, vertex );
	}
}

static void ParseStaticGeometryFaces( ApeWorld *world, PLFile *file, int32_t version )
{
	uint32_t numFaces = PL_READUINT32( file, false, NULL );

	for ( uint32_t i = 0; i < numFaces; ++i )
	{
		ApeWorldFace *face = PL_NEW( ApeWorldFace );

		face->edgeLoop = PlCreateLinkedList();

		if ( version >= 167 )
		{
			// plane
			face->normal = ParseVector( file );// normal
			face->offset = ParseFloat( file ); // offset
		}

		face->materialIndex = PlReadInt32( file, false, NULL );
		if ( face->materialIndex >= 0 )
		{
			face->material = PlGetVectorArrayElementAt( world->materials, face->materialIndex );
			assert( face->material != NULL );
		}
		// some texture indices are negative, which is valid

		if ( face->material == NULL )
		{
			face->material = apeGetFallbackMaterial();
		}

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

		face->bounds.mins = ( PLVector3 ){ FLT_MAX, FLT_MAX, FLT_MAX };
		face->bounds.maxs = ( PLVector3 ){ FLT_MIN, FLT_MIN, FLT_MIN };

		uint32_t numFaceVertices = PL_READUINT32( file, false, NULL );
		face->vertices           = PlCreateVectorArray( numFaceVertices );
		for ( uint32_t j = 0; j < numFaceVertices; ++j )
		{
			ApeWorldFaceVertex *faceVertex = PL_NEW( ApeWorldFaceVertex );

			int32_t worldVertexIndex = PlReadInt32( file, false, NULL );
			assert( worldVertexIndex >= 0 );
			if ( worldVertexIndex >= 0 )
			{
				faceVertex->u = ( ApeWorldVertex * ) PlGetVectorArrayElementAt( world->vertices, worldVertexIndex );
				assert( faceVertex->u != NULL );

				if ( faceVertex->u->position.x > face->bounds.maxs.x ) { face->bounds.maxs.x = faceVertex->u->position.x; }
				if ( faceVertex->u->position.y > face->bounds.maxs.y ) { face->bounds.maxs.y = faceVertex->u->position.y; }
				if ( faceVertex->u->position.z > face->bounds.maxs.z ) { face->bounds.maxs.z = faceVertex->u->position.z; }

				if ( faceVertex->u->position.x < face->bounds.mins.x ) { face->bounds.mins.x = faceVertex->u->position.x; }
				if ( faceVertex->u->position.y < face->bounds.mins.y ) { face->bounds.mins.y = faceVertex->u->position.y; }
				if ( faceVertex->u->position.z < face->bounds.mins.z ) { face->bounds.mins.z = faceVertex->u->position.z; }

				face->origin = PlGetAabbAbsOrigin( &face->bounds, pl_vecOrigin3 );
			}

			faceVertex->textureU = PlReadFloat32( file, false, NULL );
			assert( !isnan( faceVertex->textureU ) && ( faceVertex->textureU * faceVertex->textureU >= 0.0f ) );
			faceVertex->textureV = PlReadFloat32( file, false, NULL );
			assert( !isnan( faceVertex->textureV ) && ( faceVertex->textureV * faceVertex->textureV >= 0.0f ) );

			if ( lightmapIndex >= 0 )
			{
				faceVertex->lightmapU = ParseFloat( file );
				faceVertex->lightmapV = ParseFloat( file );
			}

			PlInsertLinkedListNode( face->edgeLoop, faceVertex );

			PlPushBackVectorArrayElement( face->vertices, faceVertex );
		}

		PlPushBackVectorArrayElement( room->faces, face );
	}
}

static void ParseStaticGeometryLightmaps( ApeWorld *world, PLFile *file )
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

		float xPerMeter = ParseFloat( file );                // x pixels per meter
		float yPerMeter = ParseFloat( file );                // y pixels per meter

		PLVector3 min = ParseVector( file );                 // min
		PLVector3 max = ParseVector( file );                 // max

		ParseVector( file );                                 // eq
		ParseFloat( file );                                  // offset
		PlReadInt32( file, false, NULL );                    // should smooth
		PlReadInt32( file, false, NULL );                    // fullbright
		PlReadInt32( file, false, NULL );                    // dropped coefficient
		PlReadInt32( file, false, NULL );                    // u coefficient
		PlReadInt32( file, false, NULL );                    // v coefficient
		ParseFloat( file );                                  // uv add x
		ParseFloat( file );                                  // uv add y
		ParseFloat( file );                                  // uv scale x
		ParseFloat( file );                                  // uv scale y

		int32_t roomIndex = PlReadInt32( file, false, NULL );// room index
		assert( PlGetVectorArrayElementAt( world->rooms, roomIndex ) != NULL );
	}
}

static void ParseStaticGeometryTextureMovers( ApeWorld *world, PLFile *file )
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

		float uPanSpeed = ParseFloat( file );
		float vPanSpeed = ParseFloat( file );
	}
}

static uint32_t GetTotalVertsForRoom( ApeWorldRoom *room, bool detail )
{
	// determine the total number of vertices

	uint32_t numVerts = 0;
	for ( uint32_t j = 0; j < PlGetNumVectorArrayElements( room->faces ); ++j )
	{
		ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != NULL );
		if ( face == NULL )
		{
			continue;
		}

		numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
	}

	if ( detail )
	{
		for ( uint32_t j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
			assert( detailRoom != NULL );
			if ( detailRoom == NULL )
			{
				continue;
			}

			for ( uint32_t k = 0; k < PlGetNumVectorArrayElements( detailRoom->faces ); ++k )
			{
				ApeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
				assert( face != NULL );
				if ( face == NULL )
				{
					continue;
				}

				numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
			}
		}
	}

	return numVerts;
}

static uint32_t GetTotalFacesForRoom( ApeWorldRoom *room, bool detail )
{
	uint32_t numFaces = PlGetNumVectorArrayElements( room->faces );

	if ( detail )
	{
		for ( uint32_t j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
			assert( detailRoom != NULL );
			if ( detailRoom == NULL )
			{
				continue;
			}

			numFaces += PlGetNumVectorArrayElements( detailRoom->faces );
		}
	}

	return numFaces;
}

static void SetupRoomSubMeshes( const ApeWorld *world, ApeWorldRoom *room )
{
	unsigned int numMaterials = PlGetNumVectorArrayElements( world->materials );

	room->numBatches = numMaterials + APE_WORLD_ROOM_NUM_BUILTIN_BATCHES;
	room->batches    = PL_NEW_( ApeWorldBatch, room->numBatches );

	unsigned int maxFaces = GetTotalFacesForRoom( room, true );
	for ( unsigned int i = 0; i < room->numBatches; ++i )
	{
		room->batches[ i ].maxSubMeshes   = maxFaces;
		room->batches[ i ].firstSubMeshes = PL_NEW_( int, room->batches[ i ].maxSubMeshes );
		room->batches[ i ].subMeshes      = PL_NEW_( int, room->batches[ i ].maxSubMeshes );
		room->batches[ i ].material       = PlGetVectorArrayElementAt( world->materials, i );
	}

	room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ]   = numMaterials;
	room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] = numMaterials + 1;
}

static void CacheRoomMesh( const ApeWorld *world, ApeWorldRoom *room )
{
	if ( room->mesh == NULL )
	{
		room->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_FAN, PLG_DRAW_STATIC, 0, GetTotalVertsForRoom( room, true ) );
		assert( room->mesh != NULL );
		if ( room->mesh == NULL )
		{
			PRINT_WARNING( "Failed to create mesh for room: %s\n", PlGetError() );
			return;
		}
	}
	else
	{
		PlgClearMesh( room->mesh );
	}

	SetupRoomSubMeshes( world, room );

	uint32_t total    = 0;
	uint32_t numFaces = GetTotalFacesForRoom( room, false );
	for ( uint32_t j = 0; j < numFaces; ++j )
	{
		ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != NULL );
		if ( face == NULL || face->materialIndex < 0 )
		{
			continue;
		}

		PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
		while ( faceVertexNode != NULL )
		{
			ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
			assert( vertex->u != NULL );

			PlgAddMeshVertex( room->mesh,
			                  &( PLVector3 ){ vertex->u->position.x, vertex->u->position.y, vertex->u->position.z },
			                  // &( PLVector3 ){ vertex->u->normal.x, vertex->u->normal.y, vertex->u->normal.z },
			                  &( PLVector3 ){ face->normal.x, face->normal.y, face->normal.z },
			                  &room->colour,
			                  &( PLVector2 ){ vertex->textureU, vertex->textureV } );

			faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
		}

		uint32_t numVertices = PlGetNumLinkedListNodes( face->edgeLoop );

		ApeWorldBatch *subMesh;
		subMesh = &room->batches[ face->materialIndex ];
		assert( subMesh->numSubMeshes != subMesh->maxSubMeshes );
		subMesh->subMeshes[ subMesh->numSubMeshes ]      = ( int ) numVertices;
		subMesh->firstSubMeshes[ subMesh->numSubMeshes ] = ( int ) total;
		subMesh->numSubMeshes++;

		subMesh = &room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ] ];
		assert( subMesh->numSubMeshes != subMesh->maxSubMeshes );
		subMesh->subMeshes[ subMesh->numSubMeshes ]      = ( int ) numVertices;
		subMesh->firstSubMeshes[ subMesh->numSubMeshes ] = ( int ) total;
		subMesh->numSubMeshes++;

		total += numVertices;
	}

	for ( uint32_t j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
	{
		ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
		assert( detailRoom != NULL );
		if ( detailRoom == NULL )
			continue;

		numFaces = PlGetNumVectorArrayElements( detailRoom->faces );
		for ( uint32_t k = 0; k < numFaces; ++k )
		{
			ApeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
			assert( face != NULL );
			if ( face == NULL || face->materialIndex < 0 )
				continue;

			PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
			while ( faceVertexNode != NULL )
			{
				ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
				assert( vertex->u != NULL );

				PlgAddMeshVertex( room->mesh,
				                  &( PLVector3 ){ vertex->u->position.x, vertex->u->position.y, vertex->u->position.z },
				                  // &( PLVector3 ){ vertex->u->normal.x, vertex->u->normal.y, vertex->u->normal.z },
				                  &( PLVector3 ){ face->normal.x, face->normal.y, face->normal.z },
				                  &room->colour,
				                  &( PLVector2 ){ vertex->textureU, vertex->textureV } );

				faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
			}

			uint32_t numVertices = PlGetNumLinkedListNodes( face->edgeLoop );

			ApeWorldBatch *subMesh;
			subMesh = &room->batches[ face->materialIndex ];
			assert( subMesh->numSubMeshes != subMesh->maxSubMeshes );
			subMesh->subMeshes[ subMesh->numSubMeshes ]      = ( int ) numVertices;
			subMesh->firstSubMeshes[ subMesh->numSubMeshes ] = ( int ) total;
			subMesh->numSubMeshes++;

			subMesh = &room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] ];
			assert( subMesh->numSubMeshes != subMesh->maxSubMeshes );
			subMesh->subMeshes[ subMesh->numSubMeshes ]      = ( int ) numVertices;
			subMesh->firstSubMeshes[ subMesh->numSubMeshes ] = ( int ) total;
			subMesh->numSubMeshes++;

			total += numVertices;
		}
	}

	PlgGenerateMeshNormals( room->mesh, true );
	PlgGenerateVertexTangentBasis( room->mesh->vertices, room->mesh->num_verts );

	PlgUploadMesh( room->mesh );

	room->isMeshCached = true;
}

static ApeWorld *ParseStaticGeometryChunk( ApeWorld *world, PLFile *file, int32_t version )
{
	if ( version >= 200 )
	{
		PL_READUINT32( file, false, NULL );// version, unused
	}
	if ( version >= 200 )
	{
		PL_READUINT32( file, false, NULL );// "modifiability" - unused
	}

	// unused string
	uint16_t size = PL_READUINT16( file, false, NULL );
	PlFileSeek( file, size, PL_SEEK_CUR );

	if ( version < 200 )
	{
		PL_READUINT32( file, false, NULL );// "modifiability" - unused
	}

	ParseStaticGeometryTextures( world, file );

	if ( version >= 180 )
	{
		uint32_t numScrollingFaces = PL_READUINT32( file, false, NULL );
		for ( uint32_t i = 0; i < numScrollingFaces; ++i )
		{
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

	// create cached room geometry
	srand( PlGetNumVectorArrayElements( world->rooms ) );
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
	{
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
		assert( room != NULL );
		if ( room == NULL || room->isDetail || room->isMeshCached )
		{
			continue;
		}

		CacheRoomMesh( world, room );
	}

	return NULL;
}

static void ParseLightsChunk( ApeWorld *world, PLFile *file, int32_t version )
{
	int32_t numLights = PlReadInt32( file, false, NULL );
	world->lights     = PlCreateVectorArray( numLights );
	for ( int32_t i = 0; i < numLights; ++i )
	{
		ApeLight *light = PL_NEW( ApeLight );

		PlReadInt32( file, false, NULL );// id

		uint16_t size;
		char *tmp = ParseString( file, &size );// class name
		PL_DELETE( tmp );

		light->position = ParseVector( file );

		ParseMat3( file );               // rotation

		tmp = ParseString( file, &size );// script name
		PL_DELETE( tmp );

		light->isHidden = PL_READUINT8( file, NULL );        // hidden in editor
		light->flags    = PL_READUINT32( file, false, NULL );// flags

		PLColour colour = ParseColour( file );
		light->colour   = PlColourU8ToF32( &colour );

		light->radius = ParseFloat( file );// * 2.0f;

		ParseFloat( file );                // fov
		ParseFloat( file );                // fov dropoff
		ParseFloat( file );                // intensity at max range
		PlReadInt32( file, false, NULL );  // dropoff type
		ParseFloat( file );                // tube light width
		ParseFloat( file );                // on intensity
		ParseFloat( file );                // on time
		ParseFloat( file );                // on time variation
		ParseFloat( file );                // off intensity
		ParseFloat( file );                // off time
		ParseFloat( file );                // off time variation

		PlPushBackVectorArrayElement( world->lights, light );
	}
}

static void ParsePlayerStart( ApeWorld *world, PLFile *file, int32_t version )
{
	world->startPosition    = ParseVector( file );
	world->startOrientation = ParseMat3( file );
}

static ApeWorld *ParseWorldFile( PLFile *file )
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

	ApeWorld *world = apeCreateWorld();

	// read in all the chunks
	uint32_t numChunks      = PL_READUINT32( file, false, NULL );
	uint32_t totalChunkSize = PL_READUINT32( file, false, NULL );
	if ( version > 161 )
	{
		uint16_t size;
		world->name = ParseString( file, &size );
	}
	if ( version >= 178 && version < 272 )
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
			case APE_WORLD_CHUNK_GEOMETRY:
				ParseStaticGeometryChunk( world, file, version );
				break;
			case APE_WORLD_CHUNK_LIGHTS:
				ParseLightsChunk( world, file, version );
				break;
			case APE_WORLD_CHUNK_PLAYERSTART:
				ParsePlayerStart( world, file, version );
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

ApeWorld *apeLoadWorld( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to load world: %s\n", PlGetError() );
		return NULL;
	}

	ApeWorld *world = ParseWorldFile( file );

	PlCloseFile( file );

	return world;
}

bool apeSaveWorld( ApeWorld *world, const char *path )
{
	world->lastSaveTime = time( NULL );

	NdBranch *root = ndPushBackObject( NULL, "world" );

	apeSerializeWorld( world, root );
	snprintf( world->path, sizeof( world->path ), "%s", path );

	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
	{
		PRINT_WARNING( "Failed to save world (%s): %s\n", path, ndGetErrorMessage() );
		return false;
	}

	return true;
}

void apeDestroyWorld( ApeWorld *world )
{
	if ( world == NULL )
	{
		return;
	}

	apeShutdownWorldVisibilitySystem_();

	if ( world->materials != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
		{
			ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
			if ( material == NULL )
			{
				continue;
			}
			apeReleaseMaterial( material );
			material = NULL;
		}
		PlDestroyVectorArray( world->materials );
		world->materials = NULL;
	}

	if ( world->rooms != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			if ( room == NULL )
			{
				continue;
			}
			apeDestroyWorldRoom( room );
			room = NULL;
		}
		PlDestroyVectorArray( world->rooms );
		world->rooms = NULL;
	}

	if ( world->portals != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->portals ); ++i )
		{
			ApeWorldPortal *portal = PlGetVectorArrayElementAt( world->portals, i );
			if ( portal == NULL )
			{
				continue;
			}
			PL_DELETE( portal );
			portal = NULL;
		}
		PlDestroyVectorArray( world->portals );
		world->portals = NULL;
	}

	if ( world->vertices != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->vertices ); ++i )
		{
			ApeWorldVertex *vertex = PlGetVectorArrayElementAt( world->vertices, i );
			if ( vertex == NULL )
			{
				continue;
			}
			PlDestroyVectorArray( vertex->adjacentFaces );
			PL_DELETE( vertex );
		}
		PlDestroyVectorArray( world->vertices );
		world->vertices = NULL;
	}


	if ( world->lights != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->lights ); ++i )
		{
			ApeLight *light = PlGetVectorArrayElementAt( world->lights, i );
			if ( light == NULL )
			{
				continue;
			}

			PL_DELETE( light );
		}

		PlDestroyVectorArray( world->lights );
		world->lights = NULL;
	}
}

void apeSpawnWorldEntities( ApeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entities );
	while ( node != NULL )
	{
		ApeWorldEntity *worldEntity = ( ApeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		apeCreateEntityFromPrefab( worldEntity->entityTemplate->name );
		node = PlGetNextLinkedListNode( node );
	}
}

/****************************************
 * Global World Properties
 ****************************************/

NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName )
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

ApeLight *YnCore_WorldSector_GetVisibleLights( ApeWorldRoom *sector, unsigned int *numLights )
{
	// TODO: for now we're just going to return this static list...
	static ApeLight lights[] = {
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
ApeWorldRoom *apeGetRoomAtPosition( ApeWorld *world, const PLVector3 *position )
{
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
	{
		ApeWorldRoom *room = ( ApeWorldRoom * ) PlGetVectorArrayElementAt( world->rooms, i );
		if ( !PlIsPointIntersectingAabb( &room->bounds, *position ) )
		{
			continue;
		}

		return room;
	}

	return NULL;
}

static void WorldSaveCallback( unsigned int argc, char **argv )
{
	ApeWorld *world = apeGetCurrentWorld();
	if ( world == NULL )
	{
		PRINT_WARNING( "No active world, can't save!\n" );
		return;
	}

	const char *dataPath = comGetDataDirectory();

	NdBranch *root = ndPushBackObject( NULL, "world" );

	apeSerializeWorld( world, root );
}

void apeRegisterWorldConsole_( void )
{
	PlRegisterConsoleVariable( "world/draw", "Toggle rendering of world.", "true", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/drawDetailRooms", "Toggle rendering of detail rooms within rooms.", "true", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showRoomColours", "Highlights each room in colour.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showRoomVolumes", "Toggle rendering of room volumes.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showPortals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.world.showPortals, NULL, false );
	PlRegisterConsoleVariable( "world/sortLights", "Sort lights before drawing world.", "true", PL_VAR_BOOL, &ape_config_.world.sortLights, NULL, false );
	PlRegisterConsoleVariable( "world/forceSimple", "Force simple render pass of world.", "false", PL_VAR_BOOL, NULL, NULL, false );

	PlRegisterConsoleCommand( "world/save", "Save the current world with the specified name.", 1, WorldSaveCallback );
}

void apeTickClientWorld_( void )
{
	apeBuildWorldVisibiltyLists_();
}
