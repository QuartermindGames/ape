// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "../cook.h"

#include "model.h"

#include "plcore/pl_parse.h"
#include "ape/ape_formats.h"
#include "plcore/pl_hashtable.h"

#define MAX_TOKEN 64

#define SMD_VERSION 1

static SmdModel *parse_smd( const char *path, const char *p )
{
	SmdModel *model = PL_NEW( SmdModel );

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
			{
				ERROR( "Expected \"version\" but found \"%s\"!\n", token );
			}

			int version = PlParseInteger( &p, nullptr );
			if ( version != SMD_VERSION )
			{
				ERROR( "Expected version %d, but found \"%d\"!\n", version, SMD_VERSION );
			}

			PlSkipLine( &p );

			isValidated = true;
			continue;
		}

		// skip nodes for now
		if ( strcmp( token, "nodes" ) == 0 )
		{
			PlSkipLine( &p );
			while ( *p != '\0' )
			{
				PlParseToken( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
				{
					break;
				}

				PlSkipLine( &p );
			}

			PlSkipLine( &p );
			continue;
		}

		// skip skeleton too...
		if ( strcmp( token, "skeleton" ) == 0 )
		{
			PlSkipLine( &p );
			while ( *p != '\0' )
			{
				PlParseToken( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
				{
					break;
				}

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
				{
					break;
				}

				char material[ MAX_TOKEN ];
				snprintf( material, sizeof( material ), "%s", token );

				PlSkipLine( &p );

				// figure out what slot it falls into

				SmdMesh *smdMesh = NULL;
				for ( unsigned int i = 0; i < SMD_MAX_MESHES; ++i )
				{
					if ( *model->meshes[ i ].material == '\0' )
					{
						// setup a new slot
						smdMesh = &model->meshes[ i ];
						snprintf( smdMesh->material, sizeof( smdMesh->material ), "%s", material );
						pl_strtolower( smdMesh->material );
						model->numMeshes++;
						break;
					}

					if ( pl_strcasecmp( model->meshes[ i ].material, material ) != 0 )
					{
						continue;
					}

					smdMesh = &model->meshes[ i ];
					break;
				}

				if ( smdMesh == nullptr )
				{
					ERROR( "Failed to fetch mesh for material \"%s\"!\n", material );
				}

				for ( unsigned int i = 0; i < 3; ++i )
				{
					PlParseInteger( &p, nullptr );// bone index

					PLVector3 position;
					position.x = PlParseFloat( &p, nullptr );
					position.y = PlParseFloat( &p, nullptr );
					position.z = PlParseFloat( &p, nullptr );
					smdMesh->triangles[ smdMesh->numTriangles ].vertices[ i ].position = position;

					PLVector3 normal;
					normal.x = PlParseFloat( &p, nullptr );
					normal.y = PlParseFloat( &p, nullptr );
					normal.z = PlParseFloat( &p, nullptr );
					smdMesh->triangles[ smdMesh->numTriangles ].vertices[ i ].normal = normal;

					PLVector2 uv;
					uv.x = PlParseFloat( &p, nullptr );
					uv.y = PlParseFloat( &p, nullptr ) * -1;// inverse, because aaargh
					smdMesh->triangles[ smdMesh->numTriangles ].vertices[ i ].uv = uv;

					PlSkipLine( &p );
				}

				smdMesh->numTriangles++;
			}

			continue;
		}

		printf( "Unhandled token, \"%s\"! Skipping line\n", token );
		PlSkipLine( &p );
	}

	return model;
}

SmdModel *model_smd_load( const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == nullptr )
	{
		ERROR( "Failed to load SMD \"%s\"!\nPL: %s\n", path, PlGetError() );
	}

	const char *p = ( char * ) PlGetFileData( file );
	if ( *p == '\0' )
	{
		ERROR( "SMD \"%s\" is empty!\n", path );
	}

	SmdModel *model = parse_smd( path, p );

	PlCloseFile( file );

	return model;
}

void model_smd_destroy( SmdModel *model )
{
	PL_DELETE( model );
}

static ApeFormatModel *smd_to_ape( const SmdModel *smd, ApeFormatModel *out )
{
	out->numMeshes = smd->numMeshes;
	if ( out->numMeshes >= APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		WARN( "Hit maximum mesh limit (%u >= %u)!\n", out->numMeshes, APE_FORMAT_MODEL_MAX_MATERIALS );
		out->numMeshes = ( APE_FORMAT_MODEL_MAX_MATERIALS - 1 );
	}

	out->numBones = smd->numBones;
	if ( out->numBones >= APE_FORMAT_MODEL_MAX_BONES )
	{
		WARN( "Hit maximum bone limit (%u >= %u)!\n", out->numBones, APE_FORMAT_MODEL_MAX_BONES );
		out->numBones = ( APE_FORMAT_MODEL_MAX_BONES - 1 );
	}

	for ( unsigned int i = 0; i < out->numBones; ++i )
	{
		out->bones[ i ].parent = ( smd->bones[ i ].parent - &smd->bones[ i ] );
		snprintf( out->bones[ i ].name, sizeof( out->bones[ i ].name ), "%s", smd->bones[ i ].name );
	}

	for ( unsigned int i = 0; i < out->numMeshes; ++i )
	{
		ApeFormatMesh *mesh = &out->meshes[ i ];

		PLPath tmp;
		strcpy( tmp, smd->meshes[ i ].material );
		pl_strntolower( tmp, sizeof( tmp ) );
		char *c = strrchr( tmp, '.' );
		if ( c != nullptr )
		{
			*c = '\0';
		}

		if ( *out->materialPath != '\0' )
		{
			PlSetupPath( mesh->material, true, "materials/%s/%s.mat.n", out->materialPath, tmp );
		}
		else
		{
			PlSetupPath( mesh->material, true, "materials/%s.mat.n", tmp );
		}

		mesh->numTriangles = smd->meshes[ i ].numTriangles;
		if ( mesh->numTriangles >= APE_FORMAT_MODEL_MAX_TRIANGLES )
		{
			WARN( "Hit maximum triangle limit (%u >= %u)!\n", mesh->numTriangles, APE_FORMAT_MODEL_MAX_TRIANGLES );
			mesh->numTriangles = ( APE_FORMAT_MODEL_MAX_TRIANGLES - 1 );
		}

		for ( unsigned int tri = 0; tri < mesh->numTriangles; ++tri )
		{
			const SmdTriangle *smdTriangle = &smd->meshes[ i ].triangles[ tri ];
			for ( unsigned int vtx = 0; vtx < 3; ++vtx )
			{
				ApeFormatVertex *vertex = &out->vertices[ tri + vtx ];
				vertex->position = smdTriangle->vertices[ vtx ].position;
				vertex->normal = smdTriangle->vertices[ vtx ].normal;
				vertex->uv = smdTriangle->vertices[ vtx ].uv;

				vertex->numWeights = smdTriangle->vertices[ vtx ].numWeights;
				for ( unsigned int wei = 0; wei < smdTriangle->vertices[ vtx ].numWeights; ++wei )
				{
					vertex->weights[ wei ].bone = ( smdTriangle->vertices[ vtx ].weights[ wei ].node - smd->bones );
					vertex->weights[ wei ].weight = smdTriangle->vertices[ vtx ].weights[ wei ].value;
				}

				mesh->triangles[ tri ].indices[ vtx ] = ( tri + vtx );
			}
		}
	}

	return out;
}

static CookModel *load_smd( const char *path ) { return ( CookModel * ) model_smd_load( path ); }
static ApeFormatModel *conv_smd( const CookModel *model, ApeFormatModel *out ) { return smd_to_ape( ( const SmdModel * ) model, out ); }
static void destroy_smd( CookModel *model ) { model_smd_destroy( ( SmdModel * ) model ); }

const CookModelFormatInterface modelSmdInterface = { "smd", load_smd, conv_smd, destroy_smd };
