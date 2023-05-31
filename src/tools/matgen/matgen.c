// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: For when you just can't be bothered to do it by hand...

#include <plcore/pl.h>

#include <yin/node.h>

static void GenerateMaterial( const char *path, PL_UNUSED void *user )
{
	printf( "Generating material for \"%s\"\n", path );

	// copy the name into a buffer we can switch out the extension
	char *name = PL_NEW_( char, strlen( PlGetFileName( path ) ) + 1 );
	strcpy( name, PlGetFileName( path ) );

	// search for the extension
	char *c = strrchr( name, '.' );
	if ( c == NULL )
	{
		printf( "Failed to fetch file extension! Skipping...\n" );
		PL_DELETE( name );
		return;
	}

	*c = '\0';

	// now build the node tree for the material
	NdBranch *root = ndPushBackObject( NULL, "material" );
	{
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

	PLPath writePath;
	PlSetupPath( writePath, true, "materials/world/%s.mat.n", name );

	PL_DELETE( name );

	// write it out and destroy it
	ndWriteFile( writePath, root, ND_FILE_UTF8 );
	ndDestroyBranch( root );
}

int main( int argc, char **argv )
{
	PlInitialize( argc, argv );

	PlScanDirectory( "materials/world", "png", GenerateMaterial, false, NULL );
	PlScanDirectory( "materials/world", "tga", GenerateMaterial, false, NULL );

	return EXIT_SUCCESS;
}
