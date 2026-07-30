// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: For when you just can't be bothered to do it by hand...

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_math.h>

#include "acm/acm.h"

#include "qmos/public/qm_os_memory.h"

#include "ape/ape_formats.h"

// uurrgghh...
#include "game/public/game/game_public.h"

static unsigned int numMaterialsGenerated = 0;

typedef struct MatGen
{
	const char *dir;
	const char *shader;
	bool        overwrite;

	const char *filterMode;

	GameMaterialSurface *surfaceLookup;
	int8_t               numSurfaces;
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

static void GenerateMaterial( const char *path, [[maybe_unused]] void *user )
{
	printf( "%s - ", path );

	const char *filename = PlGetFileName( path );
	// copy the name into a buffer we can switch out the extension
	char *name = QM_OS_MEMORY_NEW_( char, strlen( filename ) + 1 );
	strcpy( name, filename );

	// search for the extension
	char *c = strrchr( name, '.' );
	if ( c == NULL )
	{
		qm_os_memory_free( name );
		printf( "Failed to fetch file extension! Skipping...\n" );
		return;
	}

	*c = '\0';

	PLPath writePath;
	PlSetupPath( writePath, true, "%s/%s." APE_FORMAT_MATERIAL_EXTENSION, matGen.dir, name );
	if ( matGen.overwrite || ( !matGen.overwrite && !qm_fs_check_file_exists( writePath ) ) )
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
		AcmBranch *root = acm_push_object( nullptr, "material" );
		{
			if ( surfaceType != GAME_MATERIAL_SURFACE_TYPE_NONE )
			{
				acm_push_i8( root, "surfaceType", surfaceType );
			}

			AcmBranch *passesArray = acm_push_array_object( root, "passes" );
			{
				{
					AcmBranch *pass = acm_push_object( passesArray, nullptr );
					if ( matGen.filterMode != NULL )
					{
						acm_push_string( pass, "textureFilterMode", matGen.filterMode, false );
					}
					acm_push_string( pass, "shaderProgram", matGen.shader, false );
					AcmBranch *parameters = acm_push_object( pass, "shaderParameters" );
					{
						acm_push_string( parameters, "diffuseMap", path, false );
					}
				}
			}
		}

		// write it out and destroy it
		acm_write_file( writePath, root, ACM_FILE_TYPE_UTF8 );
		acm_branch_destroy( root );

		numMaterialsGenerated++;
		printf( "OK [%s]\n", matGen.surfaceLookup[ surfaceType ].description );
	}
	else
	{
		printf( "Skipped\n" );
	}

	qm_os_memory_free( name );
}

static const char *DEFAULT_DIR = "materials/world";

static bool LoadSurfacesConfig( const char *path )
{
	AcmBranch *root = acm_load_file( path, "surfaces" );
	if ( root == NULL )
	{
		printf( "Failed to load surfaces config file: %s\n", acm_get_error_message() );
		return false;
	}

	matGen.numSurfaces   = ( int8_t ) acm_get_num_of_children( root );
	matGen.surfaceLookup = QM_OS_MEMORY_NEW_( GameMaterialSurface, matGen.numSurfaces );

	GameMaterialSurface *surface = matGen.surfaceLookup;
	AcmBranch           *child   = acm_get_first_child( root );
	while ( child != NULL )
	{
		snprintf( surface->description, sizeof( surface->description ),
		          "%s", acm_get_string( child, "description", "none" ) );

		AcmBranch *aliases = acm_get_child_by_name( child, "aliases" );
		if ( aliases != NULL )
		{
			surface->numAliases = acm_get_num_of_children( aliases );
			surface->aliases    = QM_OS_MEMORY_NEW_( char *, surface->numAliases );
			acm_branch_get_string_array( aliases, surface->aliases, surface->numAliases );
			//for ( uint8_t i = 0; i < surface->numAliases; ++i )
			//{
			//	printf( "alias %u: %s\n", i, surface->aliases[ i ] );
			//}
		}

		child = acm_get_next_child( child );
		surface++;
	}

	acm_branch_destroy( root );

	return true;
}

int main( int argc, char **argv )
{
	PlInitialize( argc, argv );

	printf( "MATGEN - APE's material generation tool, for the lazy!\n"
	        "------------------------------------------------------\n" );

	QM_OS_ZERO_( matGen );

	if ( !PlPathExists( "materials" ) && !PlPathExists( "textures" ) )
	{
		printf( "Couldn't find 'materials'/'textures' sub-folder, please be sure you execute matgen "
		        "from the root directory of your project!\n" );
		return EXIT_FAILURE;
	}

	if ( !LoadSurfacesConfig( "scripts/surfaces" ACM_DEFAULT_EXTENSION ) )
	{
		printf( "Failed to load surfaces config!\n" );
	}

	matGen.dir = PlGetCommandLineArgumentValueByIndex( 1 );
	if ( matGen.dir == NULL )
	{
		printf( "No directory specified, will use '%s'\n", DEFAULT_DIR );
		matGen.dir = DEFAULT_DIR;
	}

	const char *arg;
	if ( ( arg = PlGetCommandLineArgumentValue( "-s" ) ) != NULL )
	{
		matGen.shader = arg;
	}
	else
	{
		matGen.shader = "default";
	}
	if ( ( arg = PlGetCommandLineArgumentValue( "-f" ) ) != NULL )
	{
		matGen.filterMode = arg;
	}

	matGen.overwrite = PlHasCommandLineArgument( "-o" );
	bool recursive   = PlHasCommandLineArgument( "-r" );

	PlScanDirectory( matGen.dir, "png", GenerateMaterial, recursive, NULL );
	PlScanDirectory( matGen.dir, "tga", GenerateMaterial, recursive, NULL );
	PlScanDirectory( matGen.dir, "gif", GenerateMaterial, recursive, NULL );
	PlScanDirectory( matGen.dir, "bmp", GenerateMaterial, recursive, NULL );
	PlScanDirectory( matGen.dir, "jpg", GenerateMaterial, recursive, NULL );

	printf( "Done! %u materials generated\n", numMaterialsGenerated );

	return EXIT_SUCCESS;
}
