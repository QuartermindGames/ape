// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_image.h>

#include "node.h"

#include "buildconv.h"

static uint8_t palette[ 256 ][ 3 ];
static bool CachePalette( void ) {
	PLFile *file = PlOpenFile( "PALETTE.DAT", false );
	if ( file == NULL ) {
		printf( "Failed to load \"PALETTE.DAT\": %s\n", PlGetError() );
		return false;
	}

	for ( unsigned int i = 0; i < 256; ++i ) {
		palette[ i ][ 0 ] = ( PlReadInt8( file, NULL ) * 255 ) / 63;
		palette[ i ][ 1 ] = ( PlReadInt8( file, NULL ) * 255 ) / 63;
		palette[ i ][ 2 ] = ( PlReadInt8( file, NULL ) * 255 ) / 63;
	}

	// I don't think we need the rest... I'm probably wrong :p

	PlCloseFile( file );

	return true;
}

static NLNode *GenerateMaterial( const char *texturePath ) {
	NLNode *root = NL_PushBackObj( NULL, "material" );
	NLNode *pass = NL_PushBackObj( NL_PushBackObjArray( root, "passes" ), NULL );
	NL_PushBackStr( pass, "textureFilterMode", "mipmap_nearest" );
	NL_PushBackStr( pass, "shaderProgram", "base_lighting" );
	NL_PushBackStr( NL_PushBackObj( pass, "shaderParameters" ), "diffuseMap", texturePath );
	return root;
}

static void DumpART( const char *path, unsigned int *num ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		printf( "Failed to load \"%s\": %s\n", path, PlGetError() );
		return;
	}

	// First check and confirm the version
	int32_t version = PlReadInt32( file, false, NULL );
	if ( version != 1 ) {
		printf( "Encountered unexpected art version (%d != 1)!\n", version );
		return;
	}

	PlReadInt32( file, false, NULL );// "Numtiles is not really used anymore.  I wouldn't trust it." ~ Ken

	int32_t tileStart = PlReadInt32( file, false, NULL );
	int32_t tileEnd = PlReadInt32( file, false, NULL );
	uint32_t numTiles = tileEnd - tileStart + 1;

	int16_t *tileSizeX = PL_NEW_( int16_t, numTiles );
	for ( unsigned int i = 0; i < numTiles; ++i )
		tileSizeX[ i ] = PlReadInt16( file, false, NULL );

	int16_t *tileSizeY = PL_NEW_( int16_t, numTiles );
	for ( unsigned int i = 0; i < numTiles; ++i )
		tileSizeY[ i ] = PlReadInt16( file, false, NULL );

	int32_t *tileAttributes = PL_NEW_( int32_t, numTiles );
	for ( unsigned int i = 0; i < numTiles; ++i )
		tileAttributes[ i ] = PlReadInt32( file, false, NULL );

	// Right, now for the good stuff!
	for ( unsigned int i = 0; i < numTiles; ++i ) {
		PLPath dest;
		snprintf( dest, sizeof( dest ), "materials/art/%u.png", *num );
		if ( PlFileExists( dest ) ) {
			( *num )++;
			continue;
		}

		uint32_t w = tileSizeX[ i ];
		uint32_t h = tileSizeY[ i ];

		size_t size = w * h;
		if ( size == 0 ) {
			( *num )++;
			continue;
		}

		PLImage *image = PlCreateImage( NULL, w, h, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );
		PLColour *pixel = ( PLColour * ) &image->data[ 0 ][ 0 ];

		uint8_t *buffer = PL_NEW_( uint8_t, size );
		PlReadFile( file, buffer, sizeof( uint8_t ), size );
		for ( unsigned int y = 0; y < h; ++y ) {
			for ( unsigned int x = 0; x < w; ++x ) {
				uint8_t index = buffer[ y + x * h ];
				pixel->r = palette[ index ][ 0 ];
				pixel->g = palette[ index ][ 1 ];
				pixel->b = palette[ index ][ 2 ];
				pixel->a = ( index == 255 ) ? 0 : 255;
				pixel++;
			}
		}
		PL_DELETE( buffer );

		// Write out the texture
		PlWriteImage( image, dest );
		PlDestroyImage( image );

		// Write out the material
		NLNode *root = GenerateMaterial( dest );
		snprintf( dest, sizeof( dest ), "materials/art/%u.mat.n", *num );
		NL_WriteFile( dest, root, NL_FILE_UTF8 );
		NL_DestroyNode( root );

		( *num )++;
	}

	PL_DELETE( tileSizeX );
	PL_DELETE( tileSizeY );
	PL_DELETE( tileAttributes );

	PlCloseFile( file );
}

#if 0// argh fuck this...
static int32_t playerStart[ 3 ];
static int16_t playerAngle;
static int16_t playerSector;

/**
 * The below is based upon the version 7 specification - we'll convert
 * any older versions to this on load.
 */
typedef struct Sector
{
	uint16_t wallIndex;
	uint16_t numWalls;

	int32_t ceilingZ;
	int32_t floorZ;

	int16_t ceilingStat;
	int16_t floorStat;

	int16_t ceilingTexture;
	int16_t ceilingSlope;
	int8_t ceilingShade;
	uint8_t ceilingPalette;
	uint8_t ceilingXOffset;
	uint8_t ceilingYOffset;

	int16_t floorTexture;
	int16_t floorSlope;
	int8_t floorShade;
	uint8_t floorPalette;
	uint8_t floorXOffset;
	uint8_t floorYOffset;

	uint8_t visibility;

	int16_t tagLo;
	int16_t tagHi;
	int16_t extra;
} Sector;

typedef struct Wall
{
	int32_t x;
	int32_t y;

	int16_t nextWall;
	int16_t otherWall;
	int16_t nextSector;

	int16_t stat;

	int16_t texture;
	int16_t maskedTexture;

	int8_t shade;
	uint8_t palette;

	uint8_t xSize;
	uint8_t ySize;

	uint8_t xOffset;
	uint8_t yOffset;

	int16_t tagLo;
	int16_t tagHi;
	int16_t extra;
} Wall;

/**
 * Convert the given sector into a world mesh.
 */
static void SectorToWorldMesh( const char *path, const Sector *sector, const Wall *walls )
{
	NLNode *root = NL_PushBackObj( NULL, "worldMesh" );

	NLNode *materialsNode = NL_PushBackStrArray( root, "materials", NULL, 0 );

	NLNode *verticesNode = NL_PushBackF32Array( root, "vertices", NULL, 0 );

	const Wall *wall = &walls[ sector->wallIndex ];
	for ( unsigned int i = 0; i < sector->numWalls; ++i )
	{
		// position
		NL_PushBackF32( verticesNode, NULL, ( float ) wall->x / 1000.0f );
		NL_PushBackF32( verticesNode, NULL, ( float ) wall->y / 1000.0f );
		NL_PushBackF32( verticesNode, NULL, ( float ) sector->floorZ / 1000.0f );
		// normal
		NL_PushBackF32( verticesNode, NULL, 0.0f );
		NL_PushBackF32( verticesNode, NULL, 0.0f );
		NL_PushBackF32( verticesNode, NULL, 0.0f );
		// uv
		NL_PushBackF32( verticesNode, NULL, 0.0f );
		NL_PushBackF32( verticesNode, NULL, 0.0f );
		// colour
		NL_PushBackF32( verticesNode, NULL, 1.0f );
		NL_PushBackF32( verticesNode, NULL, 1.0f );
		NL_PushBackF32( verticesNode, NULL, 1.0f );
		NL_PushBackF32( verticesNode, NULL, 1.0f );

		// AND THIS IS WHERE I STOPPED BECAUSE THIS ISNT GOING TO TRANSLATE WELL AT ALL, OH WELL
		// FML...

		wall = &walls[ wall->nextWall ];
	}

	NL_WriteFile( path, root, NL_FILE_BINARY );
}

static NLNode *MAPtoWorld( const char *worldName, const Sector *sectors, unsigned int numSectors, const Wall *walls, unsigned int numWalls ) {
	NLNode *root = NL_PushBackObj( NULL, "world" );
	NL_PushBackI32( root, "version", WORLD_VERSION );

	NLNode *propertiesNode = NL_PushBackObj( root, "properties" );
	NL_DS_SerializeColourF32( propertiesNode, "ambience", &PL_COLOURF32RGB( 128, 128, 128 ) );
	NL_DS_SerializeColourF32( propertiesNode, "clearColour", &PL_COLOURF32RGB( 128, 128, 128 ) );

	NLNode *meshesNode = NL_PushBackStrArray( root, "meshes", NULL, 0 );
	NLNode *sectorsNode = NL_PushBackObjArray( root, "sectors" );
	for ( unsigned int i = 0; i < numSectors; ++i )
	{
		PLPath tmp;
		snprintf( tmp, sizeof( tmp ), "worlds/%s/meshes/%u.wsm.n", worldName, i );
		SectorToWorldMesh( tmp, &sectors[ i ], walls );

		NL_PushBackStr( meshesNode, NULL, tmp );

		NLNode *sectorNode = NL_PushBackObj( sectorsNode, "sector" );
		NL_PushBackI32( sectorNode, "mesh", ( signed ) i );
	}

	return root;
}

#	define MAP_MAX_VERSION 6
#	define MAP_MIN_VERSION 5

static void ConvertMAP( const char *path, void *user )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		printf( "Failed to load \"%s\": %s\n", path, PlGetError() );
		return;
	}

	// Make me weep Ken, make me weeeeep!!
	int32_t version = PlReadInt32( file, false, NULL );
	if ( version < MAP_MIN_VERSION || version > MAP_MAX_VERSION )
	{
		printf( "Unexpected MAP (%s) version (%d)!\n", path, version );
		PlCloseFile( file );
		return;
	}

	playerStart[ 0 ] = PlReadInt32( file, false, NULL );
	playerStart[ 1 ] = PlReadInt32( file, false, NULL );
	playerStart[ 2 ] = PlReadInt32( file, false, NULL );

	playerAngle = PlReadInt16( file, false, NULL );
	playerSector = PlReadInt16( file, false, NULL );

	unsigned int numSectors = ( unsigned ) PlReadInt16( file, false, NULL );
	Sector *sectors = PL_NEW_( Sector, numSectors );
	for ( unsigned int i = 0; i < numSectors; ++i )
	{
		switch( version )
		{
			case 6:
			case 5:
				sectors[ i ].wallIndex = ( uint16_t ) PlReadInt16( file, false, NULL );
				sectors[ i ].numWalls = ( uint16_t ) PlReadInt16( file, false, NULL );
				sectors[ i ].ceilingTexture = PlReadInt16( file, false, NULL );
				sectors[ i ].floorTexture = PlReadInt16( file, false, NULL );
				sectors[ i ].ceilingSlope = PlReadInt16( file, false, NULL );
				sectors[ i ].floorSlope = PlReadInt16( file, false, NULL );
				sectors[ i ].ceilingZ = PlReadInt32( file, false, NULL );
				sectors[ i ].floorZ = PlReadInt32( file, false, NULL );
				sectors[ i ].ceilingShade = PlReadInt8( file, NULL );
				sectors[ i ].floorShade = PlReadInt8( file, NULL );
				sectors[ i ].ceilingXOffset = PL_READUINT8( file, NULL );
				sectors[ i ].floorXOffset = PL_READUINT8( file, NULL );
				sectors[ i ].ceilingYOffset = PL_READUINT8( file, NULL );
				sectors[ i ].floorYOffset = PL_READUINT8( file, NULL );
				sectors[ i ].ceilingStat = PL_READUINT8( file, NULL );
				sectors[ i ].floorStat = PL_READUINT8( file, NULL );
				PlReadInt8( file, NULL );// ceilingpal
				PlReadInt8( file, NULL );// floorpal
				sectors[ i ].visibility = PL_READUINT8( file, NULL );
				sectors[ i ].tagLo = PlReadInt16( file, false, NULL );
				sectors[ i ].tagHi = PlReadInt16( file, false, NULL );
				sectors[ i ].extra = PlReadInt16( file, false, NULL );
				break;
			default:
				// Unsupported...
				break;
		}
	}

	unsigned int numWalls = ( unsigned ) PlReadInt16( file, false, NULL );
	Wall *walls = PL_NEW_( Wall, numWalls );
	for ( unsigned int i = 0; i < numWalls; ++i )
	{
		switch( version )
		{
			case 6:
				walls[ i ].x = PlReadInt32( file, false, NULL );
				walls[ i ].y = PlReadInt32( file, false, NULL );
				walls[ i ].nextWall = PlReadInt16( file, false, NULL );
				walls[ i ].nextSector = PlReadInt16( file, false, NULL );
				walls[ i ].otherWall = PlReadInt16( file, false, NULL );
				walls[ i ].texture = PlReadInt16( file, false, NULL );
				walls[ i ].maskedTexture = PlReadInt16( file, false, NULL );
				walls[ i ].shade = PlReadInt8( file, NULL );
				PlReadInt8( file, NULL ); // pal
				walls[ i ].stat = PlReadInt16( file, false, NULL );
				walls[ i ].xSize = PL_READUINT8( file, NULL );
				walls[ i ].ySize = PL_READUINT8( file, NULL );
				walls[ i ].xOffset = PL_READUINT8( file, NULL );
				walls[ i ].yOffset = PL_READUINT8( file, NULL );
				walls[ i ].tagLo = PlReadInt16( file, false, NULL );
				walls[ i ].tagHi = PlReadInt16( file, false, NULL );
				walls[ i ].extra = PlReadInt16( file, false, NULL );
				break;
			case 5:
				walls[ i ].x = PlReadInt32( file, false, NULL );
				walls[ i ].y = PlReadInt32( file, false, NULL );
				walls[ i ].nextWall = PlReadInt16( file, false, NULL );
				walls[ i ].texture = PlReadInt16( file, false, NULL );
				walls[ i ].maskedTexture = PlReadInt16( file, false, NULL );
				walls[ i ].shade = PlReadInt8( file, NULL );
				walls[ i ].stat = PlReadInt16( file, false, NULL );
				walls[ i ].xSize = PL_READUINT8( file, NULL );
				walls[ i ].ySize = PL_READUINT8( file, NULL );
				walls[ i ].xOffset = PL_READUINT8( file, NULL );
				walls[ i ].yOffset = PL_READUINT8( file, NULL );
				walls[ i ].nextSector = PlReadInt16( file, false, NULL );
				walls[ i ].otherWall = PlReadInt16( file, false, NULL );
				PlReadInt16( file, false, NULL );
				PlReadInt16( file, false, NULL );
				walls[ i ].tagLo = PlReadInt16( file, false, NULL );
				walls[ i ].tagHi = PlReadInt16( file, false, NULL );
				walls[ i ].extra = PlReadInt16( file, false, NULL );
			default:
				// Unsupported...
				break;
		}
	}

	PlCloseFile( file );

	// Copy the name over into a buffer
	const char *fileName = PlGetFileName( path );
	char worldName[ 64 ];
	PL_ZERO_( worldName );
	for ( unsigned int i = 0; i < 64; ++i )
	{
		if ( fileName[ i ] == '\0' || fileName[ i ] == '.' )
			break;

		worldName[ i ] = ( char ) tolower( fileName[ i ] );
	}

	PLPath tmp;
	snprintf( tmp, sizeof( tmp ), "worlds/%s/meshes", worldName );
	if ( PlCreatePath( tmp ) )
	{
		PlCreateDirectory( tmp );
		NLNode *root = MAPtoWorld( worldName, sectors, numSectors, walls, numWalls );
		if ( root != NULL )
		{
			snprintf( tmp, sizeof( tmp ), "worlds/%s/%s.wld.n", worldName, worldName );
			NL_WriteFile( tmp, root, NL_FILE_BINARY );
		}
	}
	else
		printf( "Failed to create path: %s\n", PlGetError() );

	PL_DELETE( sectors );
	PL_DELETE( walls );
}
#endif

int main( int argc, char **argv ) {
	PlInitialize( argc, argv );

	Common_Initialize();

	// Create default project setup
	//PlCreateDirectory( "sounds" );
	//PlCreateDirectory( "worlds" );

	// First, load and cache the palette
	if ( !CachePalette() )
		return EXIT_FAILURE;

	PlCreatePath( "materials/art" );

	// Convert all ART packages
	unsigned int num = 0;
	for ( unsigned int i = 0; i < 999; ++i ) {
		char tmp[ 32 ];
		snprintf( tmp, sizeof( tmp ), "TILES%.3d.ART", i );
		if ( !PlFileExists( tmp ) )
			break;

		DumpART( tmp, &num );
	}

#if 0
	// And now convert all maps
	PlScanDirectory( ".", "MAP", ConvertMAP, false, NULL );
#endif

	return EXIT_SUCCESS;
}
