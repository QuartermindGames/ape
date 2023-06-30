// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: For when you just can't be bothered to do it by hand...

#include <plcore/pl.h>

#include <yin/node.h>

// uurrgghh...
#include "../../game/public/game/game_interface.h"

static unsigned int numMaterialsGenerated = 0;

typedef struct MatGen
{
	const char *dir;
	const char *shader;
	bool overwrite;

	GameMaterialSurface *surfaceLookup;
	int8_t numSurfaces;
} MatGen;
static MatGen matGen;

int8_t GetSurfaceTypeForName( const char *name )
{
	// good ol' binary search...
	for ( int8_t i = 0; i < matGen.numSurfaces; ++i )
	{
		GameMaterialSurface *surface = &matGen.surfaceLookup[ i ];
		for ( unsigned int j = 0; j < surface->numAliases; ++j )
		{
			if ( pl_strncasecmp( surface->aliases[ j ], name, strlen( surface->aliases[ j ] ) ) != 0 )
			{
				continue;
			}

			return i;
		}
	}

	return 0;
}

static void GenerateMaterial( const char *path, PL_UNUSED void *user )
{
	printf( "%s - ", path );

	const char *filename = PlGetFileName( path );
	// copy the name into a buffer we can switch out the extension
	char *name = PL_NEW_( char, strlen( filename ) + 1 );
	strcpy( name, filename );

	// search for the extension
	char *c = strrchr( name, '.' );
	assert( c != NULL );
	if ( c == NULL )
	{
		printf( "Failed to fetch file extension! Skipping...\n" );
		PL_DELETE( name );
		return;
	}

	*c = '\0';

	PLPath writePath;
	PlSetupPath( writePath, true, "%s/%s.mat.n", matGen.dir, name );
	if ( matGen.overwrite || ( !matGen.overwrite && !PlFileExists( writePath ) ) )
	{
#if 0// old hard-coded method

        // this just mimics the RF1 material conventions
		int8_t surfaceType = GAME_MATERIAL_SURFACE_TYPE_NONE;
		if ( pl_strncasecmp( filename, "rck_", 4 ) == 0 || pl_strncasecmp( filename, "rch_", 4 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_ROCK; }
		else if ( pl_strncasecmp( filename, "mtl_", 4 ) == 0 || pl_strncasecmp( filename, "steel_", 6 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_METAL; }
		else if ( pl_strncasecmp( filename, "fle_", 4 ) == 0 || pl_strncasecmp( filename, "flesh_", 6 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_FLESH; }
		else if ( pl_strncasecmp( filename, "wtr_", 4 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_WATER; }
		else if ( pl_strncasecmp( filename, "lva_", 4 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_LAVA; }
		else if ( pl_strncasecmp( filename, "pls_", 4 ) == 0 || pl_strncasecmp( filename, "sld_", 4 ) == 0 || pl_strncasecmp( filename, "cem_", 4 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_SOLID; }
		else if ( pl_strncasecmp( filename, "gls_", 4 ) == 0 || pl_strncasecmp( filename, "sgl_", 4 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_GLASS; }
		else if ( pl_strncasecmp( filename, "snd_", 4 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_SAND; }
		else if ( pl_strncasecmp( filename, "ice_", 4 ) == 0 ) { surfaceType = GAME_MATERIAL_SURFACE_TYPE_ICE; }

#else

		int8_t surfaceType = GetSurfaceTypeForName( filename );

#endif

		// now build the node tree for the material
		NdBranch *root = ndPushBackObject( NULL, "material" );
		{
			if ( surfaceType != GAME_MATERIAL_SURFACE_TYPE_NONE )
			{
				ndPushBackI8( root, "surfaceType", surfaceType );
			}

			NdBranch *passesArray = ndPushBackObjectArray( root, "passes" );
			{
				NdBranch *pass = ndPushBackObject( passesArray, NULL );
				{
					ndPushBackString( pass, "shaderProgram", matGen.shader );
					NdBranch *parameters = ndPushBackObject( pass, "shaderParameters" );
					{
						ndPushBackString( parameters, "diffuseMap", path );
					}
				}
			}
		}

		// write it out and destroy it
		ndWriteFile( writePath, root, ND_FILE_UTF8 );
		ndDestroyBranch( root );

		numMaterialsGenerated++;
		printf( "OK [%s]\n", matGen.surfaceLookup[ surfaceType ].description );
	}
	else
	{
		printf( "Skipped\n" );
	}

	PL_DELETE( name );
}

static const char *DEFAULT_DIR = "materials/world";

static bool LoadSurfacesConfig( const char *path )
{
	NdBranch *root = ndLoadFile( path, "surfaces" );
	if ( root == NULL )
	{
		printf( "Failed to load surfaces config file: %s\n", ndGetErrorMessage() );
		return false;
	}

	matGen.numSurfaces   = ( int8_t ) ndGetNumOfChildren( root );
	matGen.surfaceLookup = PL_NEW_( GameMaterialSurface, matGen.numSurfaces );

	GameMaterialSurface *surface = matGen.surfaceLookup;
	NdBranch *child              = ndGetFirstChild( root );
	while ( child != NULL )
	{
		snprintf( surface->description, sizeof( surface->description ),
		          "%s", ndGetStringByName( child, "description", "none" ) );

		NdBranch *aliases = ndGetChildByName( child, "aliases" );
		if ( aliases != NULL )
		{
			surface->numAliases = ndGetNumOfChildren( aliases );
			surface->aliases    = PL_NEW_( char *, surface->numAliases );
			ndGetStringArray( aliases, surface->aliases, surface->numAliases );
			//for ( uint8_t i = 0; i < surface->numAliases; ++i )
			//{
			//	printf( "alias %u: %s\n", i, surface->aliases[ i ] );
			//}
		}

		child = ndGetNextChild( child );
		surface++;
	}

	ndDestroyBranch( root );

	return true;
}

int main( int argc, char **argv )
{
	PlInitialize( argc, argv );

	printf( "MATGEN - APE's material generation tool, for the lazy!\n"
	        "------------------------------------------------------\n" );

	PL_ZERO_( matGen );

	if ( !PlPathExists( "materials" ) )
	{
		printf( "Couldn't find 'materials' sub-folder, please be sure you execute matgen "
		        "from the root directory of your project!\n" );
		return EXIT_FAILURE;
	}

	if ( !LoadSurfacesConfig( "materials/surfaces.cfg.n" ) )
	{
		printf( "Failed to load surfaces config!\n" );
		return EXIT_FAILURE;
	}

	matGen.dir = PlGetCommandLineArgumentValueByIndex( 1 );
	if ( matGen.dir == NULL )
	{
		printf( "No directory specified, will use '%s'\n", DEFAULT_DIR );
		matGen.dir = DEFAULT_DIR;
	}

	const char *arg;
	if ( ( arg = PlGetCommandLineArgumentValue( "-s" ) ) != NULL ) { matGen.shader = arg; }
	else { matGen.shader = "default"; }

	matGen.overwrite = PlHasCommandLineArgument( "-o" );
	bool recursive   = PlHasCommandLineArgument( "-r" );

	PlScanDirectory( matGen.dir, "png", GenerateMaterial, recursive, NULL );
	PlScanDirectory( matGen.dir, "tga", GenerateMaterial, recursive, NULL );
	PlScanDirectory( matGen.dir, "gif", GenerateMaterial, recursive, NULL );
	PlScanDirectory( matGen.dir, "bmp", GenerateMaterial, recursive, NULL );

	printf( "Done! %u materials generated\n", numMaterialsGenerated );

	return EXIT_SUCCESS;
}
