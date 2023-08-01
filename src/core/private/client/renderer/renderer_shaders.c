// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "renderer.h"

#include <yin/node.h>

/**********************************************************/
/** Shaders **/

static PLHashTable *shaderProgramTable;
PLGShaderProgram *ape_defaultShaderPrograms_[ APE_MAX_DEFAULT_SHADERS ];

static void RegisterShaderStage( PLGShaderProgram *program, PLGShaderStageType type, const char *path, char definitions[][ PLG_MAX_DEFINITION_LENGTH ], unsigned int numDefinitions )
{
	PLFile *filePtr = PlOpenFile( path, true );
	if ( filePtr == NULL )
	{
		PRINT_ERROR( "Failed to find shader \"%s\"!\nPL: %s\n", path, PlGetError() );
	}

	PLGShaderStage *stage = PlgCreateShaderStage( type );
	PlgSetShaderStageDefinitions( stage, definitions, numDefinitions );

	size_t length = PlGetFileSize( filePtr );
	char *buffer  = PlMAllocA( length + 1 );
	PlReadFile( filePtr, buffer, length, 1 );
	buffer[ length ] = '\0';

	PlCloseFile( filePtr );

	// get the directory we're loading the file from,
	// so we can use include relative to the original file
	PLPath directory;
	PlSetupPath( directory, true, "%s", path );
	char *sep = strrchr( directory, '/' );
	if ( sep != NULL )
	{
		*sep = '\0';
	}

	PlgCompileShaderStage( stage, buffer, length, directory );
	if ( PlGetFunctionResult() != PL_RESULT_SUCCESS )
	{
		PRINT_ERROR( "Failed to register stage, \"%s\"!\nPL: %s\n", path, PlGetError() );
	}

	PlgAttachShaderStage( program, stage );

	PlFree( buffer );
}

static ApeShaderProgramIndex *ParseShaderProgram( NdBranch *root )
{
	ApeShaderProgramIndex program;
	PL_ZERO_( program );

	const char *internalName = ndGetStringByName( root, "description", NULL );
	if ( internalName != NULL )
	{
		snprintf( program.internalName, sizeof( program.internalName ), "%s", internalName );
	}
	else
	{
		PRINT_WARNING( "Shader program with no internal name provided!\n" );
		snprintf( program.internalName, sizeof( program.internalName ), "unnamed" );
	}

	if ( apeGetShaderProgramByName( internalName ) != NULL )
	{
		PRINT_WARNING( "Shader program (%s) already registered!\n", internalName );
		return NULL;
	}

	const char *vertexPath   = ndGetStringByName( root, "vertexPath", NULL );
	const char *fragmentPath = ndGetStringByName( root, "fragmentPath", NULL );

	if ( vertexPath == NULL || fragmentPath == NULL )
	{
		PRINT_WARNING( "No vertex/fragment stage defined in program!\n" );
		return NULL;
	}

	program.internalPtr = PlgCreateShaderProgram();
	if ( program.internalPtr == NULL )
	{
		PRINT_WARNING( "Failed to create shader program!\nPL: %s\n", PlGetError() );
		return NULL;
	}

	snprintf( program.shaderPaths[ PLG_SHADER_TYPE_VERTEX ], PL_SYSTEM_MAX_PATH, "%s", vertexPath );
	snprintf( program.shaderPaths[ PLG_SHADER_TYPE_FRAGMENT ], PL_SYSTEM_MAX_PATH, "%s", fragmentPath );

	/* these allow for the program to specify what
	 * definitions should be set prior to compiling
	 * the given shader. */

	char fragmentDefinitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];
	PL_ZERO( fragmentDefinitions, PLG_MAX_DEFINITION_LENGTH * PLG_MAX_DEFINITIONS );

	char vertexDefinitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];
	PL_ZERO( vertexDefinitions, PLG_MAX_DEFINITION_LENGTH * PLG_MAX_DEFINITIONS );

	unsigned int numDefinitions[ PLG_MAX_SHADER_TYPES ];
	PL_ZERO( numDefinitions, sizeof( unsigned int ) * PLG_MAX_SHADER_TYPES );

	NdBranch *child = ndGetChildByName( root, "definitions" );
	if ( child != NULL )
	{
		NdBranch *subChild;
		if ( ( subChild = ndGetChildByName( child, "fragment" ) ) != NULL )
		{
			numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = ndGetNumOfChildren( subChild );
			if ( numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] > PLG_MAX_DEFINITIONS )
			{
				numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = PLG_MAX_DEFINITIONS;
			}

			subChild = ndGetFirstChild( subChild );
			for ( unsigned int i = 0; i < numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ]; ++i )
			{
				if ( subChild == NULL )
				{
					PRINT_WARNING( "Hit an invalid child, aborting early!\n" );
					numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = i;
					break;
				}

				ndGetStr( subChild, fragmentDefinitions[ i ], PLG_MAX_DEFINITION_LENGTH );
				subChild = ndGetNextChild( subChild );
			}
		}
		if ( ( subChild = ndGetChildByName( child, "vertex" ) ) != NULL )
		{
			numDefinitions[ PLG_SHADER_TYPE_VERTEX ] = ndGetNumOfChildren( subChild );
			if ( numDefinitions[ PLG_SHADER_TYPE_VERTEX ] > PLG_MAX_DEFINITIONS )
			{
				numDefinitions[ PLG_SHADER_TYPE_VERTEX ] = PLG_MAX_DEFINITIONS;
			}

			subChild = ndGetFirstChild( subChild );
			for ( unsigned int i = 0; i < numDefinitions[ PLG_SHADER_TYPE_VERTEX ]; ++i )
			{
				if ( subChild == NULL )
				{
					PRINT_WARNING( "Hit an invalid child, aborting early!\n" );
					numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = i;
					break;
				}

				ndGetStr( subChild, vertexDefinitions[ i ], PLG_MAX_DEFINITION_LENGTH );
				subChild = ndGetNextChild( subChild );
			}
		}
	}

	RegisterShaderStage( program.internalPtr, PLG_SHADER_TYPE_VERTEX, vertexPath, vertexDefinitions, numDefinitions[ PLG_SHADER_TYPE_VERTEX ] );
	RegisterShaderStage( program.internalPtr, PLG_SHADER_TYPE_FRAGMENT, fragmentPath, fragmentDefinitions, numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] );

	if ( !PlgLinkShaderProgram( program.internalPtr ) )
	{
		PRINT_WARNING( "Failed to link shader stages!\nPL: %s\n", PlGetError() );
		PlgDestroyShaderProgram( program.internalPtr, true );
		return NULL;
	}

	/* the default pass is an optional field that can outline
	 * the initial properties that should be used during a draw.
	 * a material can of course overwrite these. */
	child = ndGetChildByName( root, "defaultPass" );
	if ( child != NULL )
	{
		/* need to assign this for variable validation */
		program.defaultPass.program = program.internalPtr;
		/* and now we can fill this out */
		apeParseMaterialPass( child, &program.defaultPass );
	}

	/* allocate and return our program index */
	ApeShaderProgramIndex *out = PL_NEW( ApeShaderProgramIndex );
	*out                       = program;
	return out;
}

static void LoadShaderProgram( const char *path, PL_UNUSED void *userData )
{
	PRINT( "Loading program: \"%s\"\n", path );

	NdBranch *root = ndLoadFile( path, "program" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Failed to load shader program \"%s\"!\nPL: %s\n", path, PlGetError() );
		return;
	}

	ApeShaderProgramIndex *program = ParseShaderProgram( root );

	ndDestroyBranch( root );

	if ( program == NULL )
	{
		PRINT_WARNING( "An error occurred while loading shader program \"%s\"!\n", path );
		return;
	}

	strncpy( program->path, path, sizeof( program->path ) );

	PlInsertHashTableNode( shaderProgramTable, program->internalName, strlen( program->internalName ), program );
}

ApeShaderProgramIndex *apeGetShaderProgramByName( const char *name )
{
	return ( ApeShaderProgramIndex * ) PlLookupHashTableUserData( shaderProgramTable, name, strlen( name ) );
}

void apeInitializeShaders_( void )
{
	shaderProgramTable = PlCreateHashTable();
	if ( shaderProgramTable == NULL )
		PRINT_ERROR( "Failed to create shader program list: %s\n", PlGetError() );

	PRINT( "Scanning for shader programs...\n" );

	PlScanDirectory( "materials/shaders", "node", LoadShaderProgram, false, NULL );
	PlScanDirectory( "materials/shaders", "n", LoadShaderProgram, false, NULL );

	PRINT( "%d shader programs indexed\n", PlGetNumHashTableNodes( shaderProgramTable ) );

	/* now fetch the default programs */
	static const char *defaultShaderNames[ APE_MAX_DEFAULT_SHADERS ] = {
	        [APE_SHADER_DEFAULT]        = "default",
	        [APE_SHADER_LIGHTING_PASS]  = "base_lighting",
	        [APE_SHADER_DEFAULT_VERTEX] = "default_vertex",
	        [APE_SHADER_DEFAULT_ALPHA]  = "default_alpha",
	        [APE_SHADER_DEFAULT_FONT]   = "font",
	        [APE_SHADER_DEFAULT_SHADOW] = "shadow",
	};
	for ( unsigned int i = 0; i < APE_MAX_DEFAULT_SHADERS; ++i )
	{
		ApeShaderProgramIndex *programIndex = apeGetShaderProgramByName( defaultShaderNames[ i ] );
		if ( programIndex == NULL )
			PRINT_ERROR( "Failed to find default shader program, \"%s\"!\n", defaultShaderNames[ i ] );

		ape_defaultShaderPrograms_[ i ] = programIndex->internalPtr;
	}
}

PLGShaderProgram *apeGetDefaultShaderProgram( ApeDefaultShaderProgram defaultShaderProgram )
{
	return ape_defaultShaderPrograms_[ defaultShaderProgram ];
}
