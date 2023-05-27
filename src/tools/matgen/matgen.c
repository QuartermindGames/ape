// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: For when you just can't be bothered to do it by hand...

#include <plcore/pl.h>

#include <yin/node.h>

static void GenerateMaterial( const char *path, PL_UNUSED void *user )
{
	printf( "generating material for \"%s\"\n", path );

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

	char *name = PL_NEW_( char, strlen( PlGetFileName( path ) ) + 1 );
	strcpy( name, PlGetFileName( path ) );
	char *c = strrchr( name, '.' );
	*c      = '\0';

	PLPath writePath;
	PlSetupPath( writePath, true, "materials/world/%s.mat.n", name );

	PL_DELETE( name );

	ndWriteFile( writePath, root, ND_FILE_UTF8 );
}

int main( int argc, char **argv )
{
	PlInitialize( argc, argv );

	PlScanDirectory( "materials/world", "png", GenerateMaterial, false, NULL );
	PlScanDirectory( "materials/world", "tga", GenerateMaterial, false, NULL );

	return EXIT_SUCCESS;
}
