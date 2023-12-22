// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "../cook.h"

#include "plcore/pl_parse.h"
#include "plgraphics/plg.h"

#define MAX_SMD_MESHES 32

#define MAX_TOKEN 64

typedef struct SMDMesh
{
	char material[ MAX_TOKEN ];
	PLGMesh *mesh;
} SMDMesh;

static PLMModel *parse_smd( const char *path, const char *p )
{
	unsigned int numMeshes = 0;
	SMDMesh smdMeshes[ MAX_SMD_MESHES ];
	memset( smdMeshes, 0, sizeof( SMDMesh ) * MAX_SMD_MESHES );

	bool isValidated = false;
	char token[ MAX_TOKEN ];
	while ( *p != '\0' )
	{
		PlParseToken( &p, token, sizeof( token ) );
		if ( *token == '\0' || ( token[ 0 ] == '/' && token[ 1 ] == '/' ) )
		{
			PlSkipLine( &p );
			continue;
		}

		if ( !isValidated )
		{
			if ( strcmp( token, "version" ) != 0 )
				ERROR( "Expected \"version\" but found \"%s\"!\n", token );

			int version = PlParseInteger( &p, NULL );
			if ( version != 1 )
				ERROR( "Expected version 1, but found \"%d\"!\n", version );

			PlSkipLine( &p );

			isValidated = true;
			continue;
		}

		/* skip nodes for now */
		if ( strcmp( token, "nodes" ) == 0 )
		{
			PlSkipLine( &p );
			while ( *p != '\0' )
			{
				PlParseToken( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
					break;

				PlSkipLine( &p );
			}

			PlSkipLine( &p );
			continue;
		}

		/* skip skeleton too... */
		if ( strcmp( token, "skeleton" ) == 0 )
		{
			PlSkipLine( &p );
			while ( *p != '\0' )
			{
				PlParseToken( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
					break;

				PlSkipLine( &p );
			}

			PlSkipLine( &p );
			continue;
		}

		if ( strcmp( token, "triangles" ) == 0 )
		{
			PlSkipLine( &p );
			while ( *p != '\0' )
			{
				/* first need to fetch the material name.
				 * smd spec suggests the extension is ignored, so we'll do the same.
				 */

				PlParseToken( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
					break;

				char material[ MAX_TOKEN ];
				snprintf( material, sizeof( material ), "%s", token );

				PlSkipLine( &p );

				for ( unsigned int i = ( unsigned int ) strlen( material ); i > 0; --i )
				{
					if ( material[ i ] != '.' )
					{
						/* convert the string to lowercase as we go
						 * as we want the lookup to be case-insensitive */
						material[ i ] = tolower( material[ i ] );
						continue;
					}

					material[ i ] = '\0';
					break;
				}

				/* figure out what slot it falls into */

				SMDMesh *smdMesh = NULL;
				for ( unsigned int i = 0; i < MAX_SMD_MESHES; ++i )
				{
					if ( *smdMeshes[ i ].material == '\0' )
					{
						/* setup a new slot */
						smdMesh = &smdMeshes[ i ];
						snprintf( smdMesh->material, sizeof( smdMesh->material ), "%s", material );
						smdMesh->mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 100, 100 );
						if ( smdMesh->mesh == NULL )
							ERROR( "Failed to create mesh!\nPL: %s\n", PlGetError() );

						numMeshes++;
						break;
					}

					if ( strcmp( smdMeshes[ i ].material, material ) != 0 )
						continue;

					smdMesh = &smdMeshes[ i ];
					break;
				}

				if ( smdMesh == NULL )
					ERROR( "Failed to fetch mesh for material \"%s\"!\n", material );

				unsigned int indices[ 3 ];
				for ( unsigned int i = 0; i < 3; ++i )
				{
					PlParseInteger( &p, NULL ); /* bone index */

					PLVector3 position;
					position.x = PlParseFloat( &p, NULL );
					position.y = PlParseFloat( &p, NULL );
					position.z = PlParseFloat( &p, NULL );

					PLVector3 normal;
					normal.x = PlParseFloat( &p, NULL );
					normal.y = PlParseFloat( &p, NULL );
					normal.z = PlParseFloat( &p, NULL );

					PLVector2 uv;
					uv.x = PlParseFloat( &p, NULL );
					uv.y = PlParseFloat( &p, NULL ) * -1; /* inverse, because aaargh */

					indices[ i ] = PlgAddMeshVertex( smdMesh->mesh, &position, &normal, &PL_COLOUR_WHITE, &uv );

					PlSkipLine( &p );
				}

				PlgAddMeshTriangle( smdMesh->mesh, indices[ 0 ], indices[ 1 ], indices[ 2 ] );
			}

			continue;
		}

		printf( "Unhandled token, \"%s\"! Skipping line\n", token );
		PlSkipLine( &p );
	}

	PLGMesh **meshes = PlCAllocA( numMeshes, sizeof( PLGMesh * ) );
	if ( meshes == NULL )
		ERROR( "Failed to create meshes container!\nPL: %s\n", PlGetError() );

	PLMModel *model = PlmCreateStaticModel( meshes, numMeshes );
	if ( model == NULL )
		ERROR( "Failed to create model container!\nPL: %s\n", PlGetError() );

	model->numMaterials = numMeshes;
	model->materials = PlMAllocA( sizeof( PLPath ) * model->numMaterials );

	for ( unsigned int i = 0; i < numMeshes; ++i )
	{
		meshes[ i ] = smdMeshes[ i ].mesh;
		meshes[ i ]->materialIndex = i;

		if ( strstr( smdMeshes[ i ].material, "/" ) != NULL || strstr( smdMeshes[ i ].material, "\\" ) != NULL )
		{
			snprintf( model->materials[ i ], sizeof( PLPath ), "%s.mat.n", pl_strtolower( smdMeshes[ i ].material ) );
			continue;
		}

		/* copy the file name to a temp buffer with the
		 * extension stripped out, so we can ensure the
		 * textures are loaded from an appropriate subdir */
		const char *fileName = PlGetFileName( path );
		size_t l = strlen( fileName ) - 3;
		char *buf = calloc( l + 1, sizeof( char ) );
		if ( buf != NULL )
		{
			snprintf( buf, l, "%s", fileName );
			snprintf( model->materials[ i ], sizeof( PLPath ), "materials/models/%s/%s.mat.n", buf, smdMeshes[ i ].material );
			pl_strtolower( model->materials[ i ] );
			free( buf );
			continue;
		}

		/* if the above failed for whatever reason... */
		snprintf( model->materials[ i ], sizeof( PLPath ), "materials/models/%s.mat.n", pl_strtolower( smdMeshes[ i ].material ) );
	}

	return model;
}

PLMModel *model_smd_load( const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
		ERROR( "Failed to load SMD \"%s\"!\nPL: %s\n", path, PlGetError() );

	const char *p = ( char * ) PlGetFileData( file );
	if ( *p == '\0' )
		ERROR( "SMD \"%s\" is empty!\n", path );

	PLMModel *model = parse_smd( path, p );

	PlCloseFile( file );

	return model;
}

bool model_smd_serialize( NdBranch *root, const char *sourcePath )
{
	PLMModel *model = model_smd_load( sourcePath );
	if ( model == NULL )
	{
		WARN( "Failed to open smd model (%s)\n", sourcePath );
		return false;
	}

	NdBranch *materialsBranch = ndPushBackStringArray( root, "materials", NULL, 0 );
	for ( unsigned int i = 0; i < model->numMaterials; ++i )
		ndPushBackString( materialsBranch, NULL, model->materials[ i ] );

	NdBranch *meshesBranch = ndPushBackObjectArray( root, "meshes" );
	for ( unsigned int i = 0; i < model->numMeshes; ++i )
	{
	}

	return true;
}
