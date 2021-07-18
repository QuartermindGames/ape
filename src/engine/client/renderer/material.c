/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plcore/pl_linkedlist.h>

#include "yin.h"
#include "renderer.h"
#include "material.h"
#include "script.h"
#include "world.h"
#include "game_interface.h"

#include "common/node.h"

static PLLinkedList *materials[ MAX_CACHE_GROUPS ];

typedef struct MaterialVariable
{
	int            programSlot;
	ScriptVariable varData;
} MaterialVariable;

typedef struct MaterialPass
{
	PLGShaderProgram *program;
	char              programName[ 64 ];
	PLGBlend          blendMode[ 2 ];
	MaterialVariable  variables[ MAX_MATERIAL_VARIABLES ];
	unsigned int      numVariables;

	bool depthTest;
} MaterialPass;

typedef struct Material
{
	char              path[ PL_SYSTEM_MAX_PATH ];
	MaterialPass      passes[ MAX_MATERIAL_PASSES ];
	unsigned int      numPasses;
	PLLinkedListNode *node;

	MEMReference mem;
} Material;

static Material *fallbackMaterial;
Material *       RM_GetFallbackMaterial( void )
{
	return fallbackMaterial;
}

void RM_InitializeMaterialSystem( void )
{
	Print( "Initializing material system\n" );

	for ( unsigned int i = 0; i < MAX_CACHE_GROUPS; ++i )
	{
		materials[ i ] = PlCreateLinkedList();
		if ( materials[ i ] == NULL )
			PrintError( "Failed to create materials list!\nPL: %s\n", PlGetError() );
	}

	/* go ahead and create the fallback material */
	fallbackMaterial = globalSystem.MAlloc( sizeof( Material ), true );
	/* setup passes */
	fallbackMaterial->numPasses                  = 1;
	fallbackMaterial->passes[ 0 ].program        = defaultShaderPrograms[ RS_SHADER_DEFAULT ];
	fallbackMaterial->passes[ 0 ].blendMode[ 0 ] = PLG_BLEND_NONE;
	fallbackMaterial->passes[ 0 ].blendMode[ 1 ] = PLG_BLEND_NONE;
	/* setup variables */
	fallbackMaterial->passes[ 0 ].numVariables                        = 1;
	fallbackMaterial->passes[ 0 ].variables[ 0 ].varData.type         = SCRIPT_VAR_TEXTURE;
	fallbackMaterial->passes[ 0 ].variables[ 0 ].varData.value.texVar = R_GetFallbackTexture();
}

void RM_ShutdownMaterialSystem( void )
{
	for ( unsigned int i = 0; i < MAX_CACHE_GROUPS; ++i )
	{
		/* clear all the cached materials for that group */
		RM_ClearMaterials( i );

		/* and now destroy the list */
		PlDestroyLinkedList( materials[ i ] );
		materials[ i ] = NULL;
	}
}

PLGShaderProgram *RM_GetMaterialShaderProgram( Material *material, unsigned int pass )
{
	if ( pass >= material->numPasses )
		return NULL;

	return material->passes[ pass ].program;
}

static bool RM_ValidateVariableType( ScriptVariableType varType, PLGShaderUniformType uniformType )
{
	static const PLGShaderUniformType match[ MAX_SCRIPT_VAR_TYPES ] =
	        {
	                [SCRIPT_VAR_FLOAT]   = PLG_UNIFORM_FLOAT,
	                [SCRIPT_VAR_INT]     = PLG_UNIFORM_INT,
	                [SCRIPT_VAR_UINT]    = PLG_UNIFORM_UINT,
	                [SCRIPT_VAR_BOOL]    = PLG_UNIFORM_BOOL,
	                [SCRIPT_VAR_DOUBLE]  = PLG_UNIFORM_DOUBLE,
	                [SCRIPT_VAR_VEC2]    = PLG_UNIFORM_VEC2,
	                [SCRIPT_VAR_VEC3]    = PLG_UNIFORM_VEC3,
	                [SCRIPT_VAR_VEC4]    = PLG_UNIFORM_VEC4,
	                [SCRIPT_VAR_TEXTURE] = PLG_UNIFORM_SAMPLER2D,
	        };

	/* built-in variables are special */
	if ( varType == SCRIPT_VAR_BUILTIN )
		return true;

	if ( varType >= MAX_SCRIPT_VAR_TYPES || match[ varType ] != uniformType )
		return false;

	return true;
}

static int RM_GetBlendModeByTag( const char *tag )
{
	static const char *blendModeTags[ PLG_MAX_BLEND_MODES ] = {
	        [PLG_BLEND_NONE]                = "none",
	        [PLG_BLEND_ZERO]                = "zero",
	        [PLG_BLEND_ONE]                 = "one",
	        [PLG_BLEND_SRC_COLOR]           = "src_color",
	        [PLG_BLEND_ONE_MINUS_SRC_COLOR] = "one_minus_src_color",
	        [PLG_BLEND_SRC_ALPHA]           = "src_alpha",
	        [PLG_BLEND_ONE_MINUS_SRC_ALPHA] = "one_minus_src_alpha",
	        [PLG_BLEND_DST_ALPHA]           = "dst_alpha",
	        [PLG_BLEND_ONE_MINUS_DST_ALPHA] = "one_minus_dst_alpha",
	        [PLG_BLEND_DST_COLOR]           = "dst_color",
	        [PLG_BLEND_ONE_MINUS_DST_COLOR] = "one_minus_dst_color",
	        [PLG_BLEND_SRC_ALPHA_SATURATE]  = "src_alpha_saturate",
	};

	for ( int i = 0; i < PLG_MAX_BLEND_MODES; ++i )
		if ( strcmp( tag, blendModeTags[ i ] ) == 0 )
			return i;

	PrintWarn( "Invalid blend mode specified, defaulting to \"none\"!\n" );

	return PLG_BLEND_NONE;
}

static int RM_GetBuiltInByTag( const char *tag )
{
	static const char *builtInTags[ MAX_MATERIAL_BUILTINS ] = {
	        [MATERIAL_BUILTIN_TIME] = "time",
	};

	for ( int i = 0; i < MAX_MATERIAL_BUILTINS; ++i )
		if ( strcmp( tag, builtInTags[ i ] ) == 0 )
			return i;

	return -1;
}

static void RM_SetupMaterialPass( MaterialPass *pass, PLGShaderProgram *program, PLGBlend blendModeA, PLGBlend blendModeB )
{
	pass->program        = program;
	pass->blendMode[ 0 ] = blendModeA;
	pass->blendMode[ 1 ] = blendModeB;
	pass->depthTest      = true;
	pass->numVariables   = 0;

	memset( pass->variables, 0, sizeof( MaterialVariable ) * MAX_MATERIAL_VARIABLES );
}

static void RM_ParseMaterialVariable( MaterialPass *pass, char *line )
{
	if ( pass == NULL )
	{
		PrintWarn( "Pass not specified in program, skipping variable!\n" );
		return;
	}

	/* get the variable type */
	char *token = strtok( line + 4, " " );
	if ( token == NULL )
	{
		PrintWarn( "Failed to get variable type!\n" );
		return;
	}

	unsigned int i                    = pass->numVariables;
	pass->variables[ i ].varData.type = SCR_GetVariableTypeByTag( token );
	if ( pass->variables[ i ].varData.type == SCRIPT_VAR_INVALID )
	{
		PrintWarn( "Invalid variable type \"%s\"!\n", token );
		return;
	}

	token = strtok( NULL, " " );
	if ( token == NULL )
	{
		PrintWarn( "Failed to get variable name!\n" );
		return;
	}

	/* copy it across */
	snprintf( pass->variables[ i ].varData.name, sizeof( pass->variables[ i ].varData.name ), "%s", token );

	pass->variables[ i ].programSlot = PlgGetShaderUniformSlot( pass->program, pass->variables[ i ].varData.name );
	if ( pass->variables[ i ].programSlot == -1 )
	{
		PrintWarn( "Failed to fetch uniform slot for variable \"%s\"!\n", pass->variables[ i ].varData.name );
		return;
	}

	/* fetch the uniform type so we can validate it, urgh */
	unsigned int uniformType = PlgGetShaderUniformType( pass->program, pass->variables[ i ].programSlot );
	if ( !RM_ValidateVariableType( pass->variables[ i ].varData.type, uniformType ) )
	{
		PrintWarn( "Material variable \"%s\" type does not match uniform type!\n", pass->variables[ i ].varData.name );
		return;
	}

	/* and now, we need to read in the actual value */

	token = strtok( NULL, " \n" );
	if ( token == NULL )
	{
		PrintWarn( "Failed to get variable \"%s\" value!\n", pass->variables[ i ].varData.name );
		return;
	}

	/* handle built-in variables */
	switch ( pass->variables[ i ].varData.type )
	{
		case SCRIPT_VAR_BUILTIN:
			pass->variables[ i ].varData.value.iVar = RM_GetBuiltInByTag( token );
			if ( pass->variables[ i ].varData.value.iVar == -1 )
				PrintWarn( "Invalid built-in type \"%s\"!\n", token );
			break;
		case SCRIPT_VAR_DOUBLE:
			pass->variables[ i ].varData.value.dVar = strtod( token, NULL );
			break;
		case SCRIPT_VAR_BOOL:
			if ( strcmp( token, "true" ) == 0 )
				pass->variables[ i ].varData.value.bVar = true;
			else
				pass->variables[ i ].varData.value.bVar = false;
			break;
		case SCRIPT_VAR_FLOAT:
			pass->variables[ i ].varData.value.fVar = strtof( token, NULL );
			break;
		case SCRIPT_VAR_INT:
			pass->variables[ i ].varData.value.iVar = strtol( token, NULL, 10 );
			break;
		case SCRIPT_VAR_UINT:
			pass->variables[ i ].varData.value.uVar = strtoul( token, NULL, 10 );
			break;
		case SCRIPT_VAR_TEXTURE:
			pass->variables[ i ].varData.value.texVar = R_LoadTexture( token );
			break;
	}

	pass->numVariables++;
}

static Material *RM_ParseMaterial( PLFile *file )
{
	char buffer[ 1024 ];

	/* first thing we should find is the identifier */
	PlReadString( file, buffer, sizeof( buffer ) );
	if ( strcmp( "material\n", buffer ) != 0 )
	{
		PrintWarn( "Invalid identifier for material!\n" );
		return NULL;
	}

	Material mat;
	memset( &mat, 0, sizeof( Material ) );

	MaterialPass *curPass = NULL;

	char *r = PlReadString( file, buffer, sizeof( buffer ) );
	while ( r != NULL )
	{
		/* render pass */
		if ( strncmp( "pass ", r, 5 ) == 0 )
		{
			char programName[ RS_PROGRAM_NAME_LENGTH ];
			if ( sscanf( r, "pass %s\n", programName ) != 1 )
			{
				PrintWarn( "Failed to read in program for pass %d\n", mat.numPasses + 1 );
				return NULL;
			}

			PLGShaderProgram *program = RS_GetShaderProgram( programName );
			if ( program == NULL )
			{
				program = defaultShaderPrograms[ RS_SHADER_DEFAULT ];
				PrintWarn( "Failed to find program \"%s\", using fallback!\n" );
			}

			curPass = &mat.passes[ mat.numPasses++ ];
			strncpy( curPass->programName, programName, sizeof( curPass->programName ) );
			RM_SetupMaterialPass( curPass, program, PLG_BLEND_DISABLE );
		}
		/* blend mode */
		else if ( strncmp( "blend ", r, 6 ) == 0 )
		{
			char blendModeNames[ 2 ][ 32 ];
			if ( sscanf( r, "blend %s %s\n", blendModeNames[ 0 ], blendModeNames[ 1 ] ) != 2 )
			{
				PrintWarn( "Failed to read in blend for pass %d\n", mat.numPasses + 1 );
				continue;
			}

			curPass->blendMode[ 0 ] = RM_GetBlendModeByTag( blendModeNames[ 0 ] );
			curPass->blendMode[ 1 ] = RM_GetBlendModeByTag( blendModeNames[ 1 ] );
		}
		/* variable */
		else if ( strncmp( "var ", r, 4 ) == 0 )
			RM_ParseMaterialVariable( curPass, r );

		if ( strcmp( "end\n", r ) == 0 )
			break;

		r = PlReadString( file, buffer, sizeof( buffer ) );
	}

	Material *out = globalSystem.MAlloc( sizeof( Material ), true );
	memcpy( out, &mat, sizeof( Material ) );
	return out;
}

static Material *RM_GetMaterial( const char *path, CacheGroup group )
{
	PLLinkedListNode *node = PlGetFirstNode( materials[ group ] );
	while ( node != NULL )
	{
		Material *material = PlGetLinkedListNodeUserData( node );
		if ( strcmp( material->path, path ) == 0 )
			return material;

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

static const char *BlendModeToString( PLGBlend mode )
{
	switch ( mode )
	{
		case PLG_BLEND_ONE_MINUS_DST_ALPHA:
			return "one_minus_dst_alpha";
		case PLG_BLEND_ONE_MINUS_DST_COLOR:
			return "one_minus_dst_color";
		case PLG_BLEND_ONE_MINUS_SRC_ALPHA:
			return "one_minus_src_alpha";
		case PLG_BLEND_ONE_MINUS_SRC_COLOR:
			return "one_minus_src_color";
		case PLG_BLEND_SRC_ALPHA:
			return "src_alpha";
		case PLG_BLEND_SRC_ALPHA_SATURATE:
			return "src_alpha_saturate";
		case PLG_BLEND_SRC_COLOR:
			return "src_color";
		case PLG_BLEND_DST_ALPHA:
			return "dst_alpha";
		case PLG_BLEND_DST_COLOR:
			return "dst_color";
		case PLG_BLEND_ONE:
			return "one";
		default:
			return "zero";
	}
}

/**
 * Temporary; convert bulk of our old .mat files to
 * the new .node format instead.
 */
static void ConvertMatToNode( const Material *material )
{
	Print( "IN: \"%s\"\n", material->path );

	char outPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( outPath, sizeof( outPath ), "%s", material->path );
	outPath[ strlen( outPath ) - 3 ] = '\0';
	strcat( outPath, "node" );

	NLNode *root = NL_LoadFile( outPath, "material" );
	if ( root != NULL )
	{
		//NL_PrintNodeTree( root, 0 );
		return;
	}

	char temp[ PL_SYSTEM_MAX_PATH ];
	strcpy( temp, outPath );
	snprintf( outPath, sizeof( outPath ), "%s%s", ComFS_GetDataDirectory(), temp );

	root              = NL_PushBackObj( NULL, "material" );
	NLNode *passArray = NL_PushBackObjArray( root, "passes" );
	for ( unsigned int i = 0; i < material->numPasses; ++i )
	{
		NLNode *pass = NL_PushBackObj( passArray, NULL );
		NL_PushBackStr( pass, "shaderProgram", material->passes[ i ].programName );
		const char *blendMode[ 2 ] = {
		        BlendModeToString( material->passes[ i ].blendMode[ 0 ] ),
		        BlendModeToString( material->passes[ i ].blendMode[ 1 ] ) };
		NL_PushBackStrArray( pass, "blendMode", blendMode, 2 );

		NL_PushBackBool( pass, "depthTest", material->passes[ i ].depthTest );

		NLNode *shaderParms = NL_PushBackObj( pass, "shaderParameters" );
		for ( unsigned int j = 0; j < material->passes[ i ].numVariables; ++j )
		{
			const MaterialVariable *var = &material->passes[ i ].variables[ j ];
			switch ( var->varData.type )
			{
				case SCRIPT_VAR_TEXTURE:
					NL_PushBackStr( shaderParms, var->varData.name, var->varData.value.texVar->path );
					break;
				case SCRIPT_VAR_BUILTIN:
					NL_PushBackI32( shaderParms, var->varData.name, var->varData.value.iVar );
					break;
				case SCRIPT_VAR_BOOL:
					NL_PushBackBool( shaderParms, var->varData.name, var->varData.value.bVar );
					break;
				case SCRIPT_VAR_DOUBLE:
					NL_PushBackF64( shaderParms, var->varData.name, var->varData.value.dVar );
					break;
				case SCRIPT_VAR_FLOAT:
					NL_PushBackF32( shaderParms, var->varData.name, var->varData.value.fVar );
					break;
				case SCRIPT_VAR_UINT:
				case SCRIPT_VAR_INT:
					NL_PushBackI32( shaderParms, var->varData.name, var->varData.value.iVar );
					break;
				case SCRIPT_VAR_STRING:
					NL_PushBackStr( shaderParms, var->varData.name, var->varData.value.strVar );
					break;
				default:
					break;
			}
		}
	}

	Print( "OUT: \"%s\"\n", outPath );

	NL_WriteFile( outPath, root, NL_FILE_ASCII );

	//NL_PrintNodeTree( root, 0 );
	NL_DestroyNode( root );
}

static void RM_CB_DestroyMaterial( void *userData )
{
	Material *material = userData;
	u_assert( material != NULL );
	if ( material == NULL )
		return;

	PLLinkedList *container = PlGetLinkedListNodeContainer( material->node );
	if ( container != NULL )
		PlDestroyLinkedListNode( container, material->node );

	globalSystem.Free( material );
}

Material *RM_CacheMaterial( const char *path, CacheGroup group, bool useFallback )
{
	/* check if it's already cached */
	Material *material = RM_GetMaterial( path, group );
	if ( material != NULL )
	{
		MEM_AddReference( &material->mem );
		return material;
	}

	/* fallback should be optional, as in some cases we might actually care */
	Material *fallbackPtr = useFallback ? fallbackMaterial : NULL;

	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		PrintWarn( "Failed to load material, \"%s\"!\nPL: %s\n", path, PlGetError() );
		return fallbackPtr;
	}

	material = RM_ParseMaterial( file );

	PlCloseFile( file );

	if ( material == NULL )
	{
		PrintWarn( "Failed to cache material, \"%s\"!\n", path );
		return fallbackPtr;
	}

	snprintf( material->path, sizeof( material->path ), "%s", path );
	material->node = PlInsertLinkedListNode( materials[ group ], material );

#if 1
	ConvertMatToNode( material );
#endif

	MEM_SetupReferenceInstance( "material", &material->mem, RM_CB_DestroyMaterial, material );
	MEM_AddReference( &material->mem );

	return material;
}

void RM_ReleaseMaterial( Material *material )
{
	u_assert( material != NULL );
	MEM_ReleaseReference( &material->mem );
}

void RM_ClearMaterials( CacheGroup group )
{
	PLLinkedListNode *node = PlGetFirstNode( materials[ group ] );
	while ( node != NULL )
	{
		globalSystem.Free( PlGetLinkedListNodeUserData( node ) );
		node = PlGetNextLinkedListNode( node );
	}

	PlDestroyLinkedListNodes( materials[ group ] );
}

static void RM_SetBuiltInVariable( PLGShaderProgram *program, int uniformSlot, int variable )
{
	if ( variable == -1 )
		return;

	switch ( variable )
	{
		case MATERIAL_BUILTIN_TIME:
		{
			unsigned int numTicks = Engine_GetNumTicks();
			PlgSetShaderUniformValueByIndex( program, uniformSlot, &numTicks, false );
			break;
		}

		default:
			break;
	}
}

static void RM_SetWorldVariables( PLGShaderProgram *program )
{
	World *world = Game_GetCurrentWorld();
	if ( world == NULL )
		return;

	/* set global uniforms, if they exist */
	int slot;
	slot = PlgGetShaderUniformSlot( program, "sun.colour" );
	if ( slot >= 0 )
	{
		PLVector4 sunColour = W_GetSunColour( world );
		PlgSetShaderUniformValueByIndex( program, slot, &sunColour, false );
	}
	slot = PlgGetShaderUniformSlot( program, "sun.position" );
	if ( slot >= 0 )
	{
		PLVector3 sunPosition = W_GetSunPosition( world );
		PlgSetShaderUniformValueByIndex( program, slot, &sunPosition, false );
	}
	slot = PlgGetShaderUniformSlot( program, "sun.ambience" );
	if ( slot >= 0 )
	{
		PLVector4 ambience = W_GetAmbience( world );
		PlgSetShaderUniformValueByIndex( program, slot, &ambience, false );
	}
}

void RM_DrawMesh( Material *material, PLGMesh *mesh )
{
	for ( unsigned int i = 0; i < material->numPasses; ++i )
	{
		MaterialPass *curPass = &material->passes[ i ];

		PlgSetShaderProgram( curPass->program );
		PlgSetBlendMode( curPass->blendMode[ 0 ], curPass->blendMode[ 1 ] );

		PlgSetShaderUniformValue( curPass->program, "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), true );

		RM_SetWorldVariables( curPass->program );

		unsigned int curUnit = 0;
		for ( unsigned int j = 0; j < curPass->numVariables; ++j )
		{
			/* textures just need to be set per their respective unit */
			if ( curPass->variables[ j ].varData.type == SCRIPT_VAR_TEXTURE )
			{
				PlgSetTexture( curPass->variables[ j ].varData.value.texVar, curUnit );
				PlgSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curUnit, false );
				curUnit++;
				continue;
			}
			/* built-in variables are special cases */
			else if ( curPass->variables[ j ].varData.type == SCRIPT_VAR_BUILTIN )
			{
				RM_SetBuiltInVariable( curPass->program, curPass->variables[ j ].programSlot, curPass->variables[ j ].varData.value.iVar );
				continue;
			}

			PlgSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curPass->variables[ j ].varData.value, false );
		}

		PlgUploadMesh( mesh );
		PlgDrawMesh( mesh );

		g_gfxPerfStats.numBatches++;
		g_gfxPerfStats.numFacesDrawn += mesh->num_triangles;
	}
}
