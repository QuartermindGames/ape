// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: For when you just can't be bothered to do it by hand...

#include <plcore/pl.h>

#include <yin/node.h>

// uurrgghh...
#include "../../game/public/game/game_interface.h"

static void GenerateMaterial( const char *path, PL_UNUSED void *user )
{
	printf( "Generating material for \"%s\"\n", path );

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
	PlSetupPath( writePath, true, "materials/world/%s.mat.n", name );
	if ( !PlFileExists( writePath ) )
	{
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
					ndPushBackString( pass, "shaderProgram", "base_lighting" );
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
	}

	PL_DELETE( name );
}

int main( int argc, char **argv )
{
	PlInitialize( argc, argv );

	PlScanDirectory( "materials/world", "png", GenerateMaterial, false, NULL );
	PlScanDirectory( "materials/world", "tga", GenerateMaterial, false, NULL );

	return EXIT_SUCCESS;
}
