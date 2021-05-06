//
// Created by hogsy on 02/05/2021.
//

#include "yin.h"
#include "renderer.h"

#include "common/node.h"

/**********************************************************/
/** Shaders **/

typedef struct ShaderProgramIndex {
	char path[ PL_SYSTEM_MAX_PATH ];
	char shaderPaths[ PLG_MAX_SHADER_TYPES ][ PL_SYSTEM_MAX_PATH ];
    char internalName[ GFX_PROGRAM_NAME_LENGTH ];
    PLGShaderProgram *internalPtr;
    PLLinkedListNode *node;
} ShaderProgramIndex;

static PLLinkedList *shaderPrograms;
PLGShaderProgram *defaultShaderPrograms[ GFX_MAX_DEFAULT_SHADERS ];

static void RS_RegisterShaderStage( PLGShaderProgram *program, PLGShaderStageType type, const char *path ) {
    PLFile *filePtr = PlOpenFile( path, true );
    if ( filePtr == NULL ) {
        PrintError( "Failed to find shader \"%s\"!\nPL: %s\n", path, PlGetError() );
    }

    const char *buffer = ( const char * ) PlGetFileData( filePtr );
    size_t length = PlGetFileSize( filePtr );

    if ( !PlgRegisterShaderStageFromMemory( program, buffer, length, type ) ) {
        PrintError( "Failed to register stage, \"%s\"!\nPL: %s\n", path, PlGetError() );
    }

    PlCloseFile( filePtr );
}

static ShaderProgramIndex *RS_ParseShaderProgram( NLNode *root ) {
    ShaderProgramIndex program;

    const char* vertexPath = NL_GetStringByName( root, "vertexPath" );
    const char *fragmentPath = NL_GetStringByName( root, "fragmentPath" );

    if ( vertexPath == NULL || fragmentPath == NULL ) {
        PrintWarn( "No vertex/fragment stage defined in program!\n" );
        return NULL;
    }

    program.internalPtr = PlgCreateShaderProgram();
    if ( program.internalPtr == NULL ) {
        PrintWarn( "Failed to create shader program!\nPL: %s\n", PlGetError() );
        return NULL;
    }

	RS_RegisterShaderStage( program.internalPtr, PLG_SHADER_TYPE_VERTEX, vertexPath );
	RS_RegisterShaderStage( program.internalPtr, PLG_SHADER_TYPE_FRAGMENT, fragmentPath );

    if ( !PlgLinkShaderProgram( program.internalPtr ) ) {
        PrintError( "Failed to link shader stages!\nPL: %s\n", PlGetError() );
    }

	snprintf( program.shaderPaths[ PLG_SHADER_TYPE_VERTEX ], PL_SYSTEM_MAX_PATH, "%s", vertexPath );
	snprintf( program.shaderPaths[ PLG_SHADER_TYPE_FRAGMENT ], PL_SYSTEM_MAX_PATH, "%s", fragmentPath );

	const char *internalName = NL_GetStringByName( root, "description" );
	if ( internalName != NULL ) {
		snprintf( program.internalName, sizeof( program.internalName ), "%s", internalName );
	} else {
		snprintf( program.internalName, sizeof( program.internalName ), "unnamed" );
	}

    /* allocate and return our program index */
    ShaderProgramIndex *out = globalSystem.MAlloc( sizeof( ShaderProgramIndex ), true );
    *out = program;
    return out;
}

#if 0
static void ConvertPlgToNode( const ShaderProgramIndex *programIndex ) {
    Print( "IN: \"%s\"\n", programIndex->path );

    char outPath[ PL_SYSTEM_MAX_PATH ];
    snprintf( outPath, sizeof( outPath ), "%s", programIndex->path );
    outPath[ strlen( outPath ) - 3 ] = '\0';
    strcat( outPath, "node" );

    NLNode *root = NL_LoadFile( outPath, "program" );
    if ( root != NULL ) {
        NL_PrintNodeTree( root, 0 );
        return;
    }

    char temp[ PL_SYSTEM_MAX_PATH ];
    strcpy( temp, outPath );
    snprintf( outPath, sizeof( outPath ), "%s%s", ComFS_GetDataDirectory(), temp );

    root = NL_PushBackObj( NULL, "program" );
    NL_PushBackString( root, "description", programIndex->internalName );
	if ( programIndex->shaderPaths[ PLG_SHADER_TYPE_VERTEX ][ 0 ] != '\0' ) {
		NL_PushBackString( root, "vertexPath", programIndex->shaderPaths[ PLG_SHADER_TYPE_VERTEX ] );
	}
	if ( programIndex->shaderPaths[ PLG_SHADER_TYPE_FRAGMENT ][ 0 ] != '\0' ) {
		NL_PushBackString( root, "fragmentPath", programIndex->shaderPaths[ PLG_SHADER_TYPE_FRAGMENT ] );
	}

    Print( "OUT: \"%s\"\n", outPath );

	NL_WriteFile( outPath, root, NL_FILE_ASCII );

	NL_PrintNodeTree( root, 0 );
	NL_DestroyNode( root );
}
#endif

static void RS_LoadShaderProgram( const char *path, void *userData ) {
	NLNode *root = NL_LoadFile( path, "program" );
	if ( root == NULL ) {
        PrintWarn( "Failed to load shader program \"%s\"!\nPL: %s\n", path, PlGetError() );
        return;
	}

	NL_PrintNodeTree( root, 0 );

    ShaderProgramIndex *program = RS_ParseShaderProgram( root );

	NL_DestroyNode( root );

    if ( program != NULL ) {
		strncpy( program->path, path, sizeof( program->path ) );
        program->node = PlInsertLinkedListNode( shaderPrograms, program );
    }
}

PLGShaderProgram *RS_GetShaderProgram( const char *name ) {
    PLLinkedListNode *root = PlGetFirstNode( shaderPrograms );
    while ( root != NULL ) {
        ShaderProgramIndex *programIndex = PlGetLinkedListNodeUserData( root );
        if ( strcmp( name, programIndex->internalName ) == 0 ) {
            return programIndex->internalPtr;
        }

        root = PlGetNextLinkedListNode( root );
    }

    return NULL;
}

void RS_InitializeShaderPrograms( void ) {
	shaderPrograms = PlCreateLinkedList();

	PlScanDirectory( "materials/shaders", "node", RS_LoadShaderProgram, false, NULL );

    Print( "%d shader programs indexed\n", PlGetNumLinkedListNodes( shaderPrograms ) );

    /* now fetch the default programs */
    const char *defaultShaderNames[ GFX_MAX_DEFAULT_SHADERS ] = {
            [GFX_SHADER_DEFAULT] = "default",
            [GFX_SHADER_LIGHTING_PASS] = "base_lighting",
            [GFX_SHADER_DEFAULT_VERTEX] = "default_vertex",
            [GFX_SHADER_DEFAULT_ALPHA] = "default_alpha",
            [GFX_SHADER_POST_PROCESS] = "postprocess",
    };
    for ( unsigned int i = 0; i < GFX_MAX_DEFAULT_SHADERS; ++i ) {
		defaultShaderPrograms[ i ] = RS_GetShaderProgram( defaultShaderNames[ i ] );
        if ( defaultShaderPrograms[ i ] == NULL ) {
            PrintError( "Failed to find default shader program, \"%s\"!\n", defaultShaderNames[ i ] );
        }
    }
}
