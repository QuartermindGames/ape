/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#include <PL/pl_llist.h>

#include "yin.h"
#include "renderer.h"
#include "material.h"
#include "script.h"

static PLLinkedList *materials[ MAX_CACHE_GROUPS ];
//static PLLinkedList *shaderPrograms;

static Material *fallbackMaterial;

typedef struct MaterialVariable {
	int programSlot;
	ScriptVariable varData;
} MaterialVariable;

typedef struct MaterialPass {
	PLShaderProgram *program;
	PLBlend blendMode[ 2 ];
	MaterialVariable variables[ MAX_MATERIAL_VARIABLES ];
	unsigned int numVariables;
} MaterialPass;

typedef struct Material {
	char path[ PL_SYSTEM_MAX_PATH ];
	MaterialPass passes[ MAX_MATERIAL_PASSES ];
	unsigned int numPasses;
	PLLinkedListNode *node;
} Material;

void RM_InitializeMaterialSystem( void ) {
	PrintMsg( "Initializing material system\n" );

	for ( unsigned int i = 0; i < MAX_CACHE_GROUPS; ++i ) {
		materials[ i ] = plCreateLinkedList();
		if ( materials[ i ] == NULL ) {
			PrintError( "Failed to create materials list!\nPL: %s\n", plGetError() );
		}
	}

	/* go ahead and create the fallback material */
	fallbackMaterial = Sys_calloc( 1, sizeof( Material ) );
	/* setup passes */
	fallbackMaterial->numPasses = 1;
	fallbackMaterial->passes[ 0 ].program = gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ];
	fallbackMaterial->passes[ 0 ].blendMode[ 0 ] = PL_BLEND_NONE;
	fallbackMaterial->passes[ 0 ].blendMode[ 1 ] = PL_BLEND_NONE;
	/* setup variables */
	fallbackMaterial->passes[ 0 ].numVariables = 1;
	fallbackMaterial->passes[ 0 ].variables[ 0 ].varData.type = SCRIPT_VAR_TEXTURE;
	fallbackMaterial->passes[ 0 ].variables[ 0 ].varData.value.texVar = Gfx_GetFallbackTexture();
}

void RM_ShutdownMaterialSystem( void ) {
	for ( unsigned int i = 0; i < MAX_CACHE_GROUPS; ++i ) {
		/* clear all the cached materials for that group */
		RM_ClearMaterials( i );

		/* and now destroy the list */
		plDestroyLinkedList( materials[ i ] );
		materials[ i ] = NULL;
	}
}

PLShaderProgram *RM_GetMaterialShaderProgram( Material *material, unsigned int pass ) {
	if ( pass >= material->numPasses ) {
		return NULL;
	}

	return material->passes[ pass ].program;
}

static bool RM_ValidateVariableType( ScriptVariableType varType, PLShaderUniformType uniformType ) {
	static const PLShaderUniformType match[ MAX_SCRIPT_VAR_TYPES ] =
	        {
	                [SCRIPT_VAR_FLOAT] = PL_UNIFORM_FLOAT,
	                [SCRIPT_VAR_INT] = PL_UNIFORM_INT,
	                [SCRIPT_VAR_UINT] = PL_UNIFORM_UINT,
	                [SCRIPT_VAR_BOOL] = PL_UNIFORM_BOOL,
	                [SCRIPT_VAR_DOUBLE] = PL_UNIFORM_DOUBLE,
	                [SCRIPT_VAR_VEC2] = PL_UNIFORM_VEC2,
	                [SCRIPT_VAR_VEC3] = PL_UNIFORM_VEC3,
	                [SCRIPT_VAR_VEC4] = PL_UNIFORM_VEC4,
	                [SCRIPT_VAR_TEXTURE] = PL_UNIFORM_SAMPLER2D,
	        };

	/* built-in variables are special */
	if ( varType == SCRIPT_VAR_BUILTIN ) {
		return true;
	}

	if ( varType >= MAX_SCRIPT_VAR_TYPES || match[ varType ] != uniformType ) {
		return false;
	}

	return true;
}

static int RM_GetBlendModeByTag( const char *tag ) {
	static const char *blendModeTags[ PL_MAX_BLEND_MODES ] = {
	        [PL_BLEND_NONE] = "none",
	        [PL_BLEND_ZERO] = "zero",
	        [PL_BLEND_ONE] = "one",
	        [PL_BLEND_SRC_COLOR] = "src_color",
	        [PL_BLEND_ONE_MINUS_SRC_COLOR] = "one_minus_src_color",
	        [PL_BLEND_SRC_ALPHA] = "src_alpha",
	        [PL_BLEND_ONE_MINUS_SRC_ALPHA] = "one_minus_src_alpha",
	        [PL_BLEND_DST_ALPHA] = "dst_alpha",
	        [PL_BLEND_ONE_MINUS_DST_ALPHA] = "one_minus_dst_alpha",
	        [PL_BLEND_DST_COLOR] = "dst_color",
	        [PL_BLEND_ONE_MINUS_DST_COLOR] = "one_minus_dst_color",
	        [PL_BLEND_SRC_ALPHA_SATURATE] = "src_alpha_saturate",
	};

	for ( int i = 0; i < PL_MAX_BLEND_MODES; ++i ) {
		if ( strcmp( tag, blendModeTags[ i ] ) == 0 ) {
			return i;
		}
	}

	PrintWarn( "Invalid blend mode specified, defaulting to \"none\"!\n" );

	return PL_BLEND_NONE;
}

static int RM_GetBuiltInByTag( const char *tag ) {
	static const char *builtInTags[ MAX_MATERIAL_BUILTINS ] = {
		[ MATERIAL_BUILTIN_TIME ] = "time",
	};

	for ( int i = 0; i < MAX_MATERIAL_BUILTINS; ++i ) {
		if ( strcmp( tag, builtInTags[ i ] ) == 0 ) {
			return i;
		}
	}

	return -1;
}

static void RM_SetupMaterialPass( MaterialPass *pass, PLShaderProgram *program, PLBlend blendModeA, PLBlend blendModeB ) {
	pass->program = program;
	pass->blendMode[ 0 ] = blendModeA;
	pass->blendMode[ 1 ] = blendModeB;
	pass->numVariables = 0;

	memset( pass->variables, 0, sizeof( MaterialVariable ) * MAX_MATERIAL_VARIABLES );
}

static void RM_ParseMaterialVariable( MaterialPass *pass, char *line ) {
	if ( pass == NULL ) {
		PrintWarn( "Pass not specified in program, skipping variable!\n" );
		return;
	}

	/* get the variable type */
	char *token = strtok( line + 4, " " );
	if ( token == NULL ) {
		PrintWarn( "Failed to get variable type!\n" );
		return;
	}

	unsigned int i = pass->numVariables;
	pass->variables[ i ].varData.type = SCR_GetVariableTypeByTag( token );
	if ( pass->variables[ i ].varData.type == SCRIPT_VAR_INVALID ) {
		PrintWarn( "Invalid variable type \"%s\"!\n", token );
		return;
	}

	token = strtok( NULL, " " );
	if ( token == NULL ) {
		PrintWarn( "Failed to get variable name!\n" );
		return;
	}

	/* copy it across */
	snprintf( pass->variables[ i ].varData.name, sizeof( pass->variables[ i ].varData.name ), "%s", token );

	pass->variables[ i ].programSlot = plGetShaderUniformSlot( pass->program, pass->variables[ i ].varData.name );
	if ( pass->variables[ i ].programSlot == -1 ) {
		PrintWarn( "Failed to fetch uniform slot for variable \"%s\"!\n", pass->variables[ i ].varData.name );
		return;
	}

	/* fetch the uniform type so we can validate it, urgh */
	unsigned int uniformType = plGetShaderUniformType( pass->program, pass->variables[ i ].programSlot );
	if ( !RM_ValidateVariableType( pass->variables[ i ].varData.type, uniformType ) ) {
		PrintWarn( "Material variable \"%s\" type does not match uniform type!\n", pass->variables[ i ].varData.name );
		return;
	}

	/* and now, we need to read in the actual value */

	token = strtok( NULL, " \n" );
	if ( token == NULL ) {
		PrintWarn( "Failed to get variable \"%s\" value!\n", pass->variables[ i ].varData.name );
		return;
	}

	/* handle built-in variables */
	switch( pass->variables[ i ].varData.type ) {
		case SCRIPT_VAR_BUILTIN:
			pass->variables[ i ].varData.value.iVar = RM_GetBuiltInByTag( token );
			if ( pass->variables[ i ].varData.value.iVar == -1 ) {
				PrintWarn( "Invalid built-in type \"%s\"!\n", token );
			}
			break;
		case SCRIPT_VAR_DOUBLE:
			pass->variables[ i ].varData.value.dVar = strtod( token, NULL );
			break;
		case SCRIPT_VAR_BOOL:
			if ( strcmp( token, "true" ) == 0 ) {
				pass->variables[ i ].varData.value.bVar = true;
			} else {
				pass->variables[ i ].varData.value.bVar = false;
			}
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
			pass->variables[ i ].varData.value.texVar = Gfx_LoadTexture( token );
			break;
	}

	pass->numVariables++;
}

static Material *RM_ParseMaterial( PLFile *file ) {
	char buffer[ 1024 ];

	/* first thing we should find is the identifier */
	plReadString( file, buffer, sizeof( buffer ) );
	if ( strcmp( "material\n", buffer ) != 0 ) {
		PrintWarn( "Invalid identifier for material!\n" );
		return NULL;
	}

	Material mat;
	memset( &mat, 0, sizeof( Material ) );

	MaterialPass *curPass = NULL;

	char *r = plReadString( file, buffer, sizeof( buffer ) );
	while( r != NULL ) {
		/* render pass */
		if ( strncmp( "pass ", r, 5 ) == 0 ) {
			char programName[ GFX_PROGRAM_NAME_LENGTH ];
			if ( sscanf( r, "pass %s\n", programName ) != 1 ) {
				PrintWarn( "Failed to read in program for pass %d\n", mat.numPasses + 1 );
				return NULL;
			}

			PLShaderProgram *program = Gfx_GetShaderProgram( programName );
			if ( program == NULL ) {
				program = gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ];
				PrintWarn( "Failed to find program \"%s\", using fallback!\n" );
			}

			curPass = &mat.passes[ mat.numPasses++ ];
			RM_SetupMaterialPass( curPass, program, PL_BLEND_DISABLE );
		}
		/* blend mode */
		else if ( strncmp( "blend ", r, 6 ) == 0 ) {
			char blendModeNames[ 2 ][ 32 ];
			if ( sscanf( r, "blend %s %s\n", blendModeNames[ 0 ], blendModeNames[ 1 ] ) != 2 ) {
				PrintWarn( "Failed to read in blend for pass %d\n", mat.numPasses + 1 );
				continue;
			}

			curPass->blendMode[ 0 ] = RM_GetBlendModeByTag( blendModeNames[ 0 ] );
			curPass->blendMode[ 1 ] = RM_GetBlendModeByTag( blendModeNames[ 1 ] );
		}
		/* variable */
		else if ( strncmp( "var ", r, 4 ) == 0 ) {
			RM_ParseMaterialVariable( curPass, r );
		}

		if ( strcmp( "end\n", r ) == 0 ) {
			break;
		}

		r = plReadString( file, buffer, sizeof( buffer ) );
	}

	Material *out = Sys_malloc( sizeof( Material ) );
	memcpy( out, &mat, sizeof( Material ) );
	return out;
}

static Material *RM_GetMaterial( const char *path, CacheGroup group ) {
	PLLinkedListNode *node = plGetRootNode( materials[ group ] );
	while ( node != NULL ) {
		Material *material = plGetLinkedListNodeUserData( node );
		if ( strcmp( material->path, path ) == 0 ) {
			return material;
		}

		node = plGetNextLinkedListNode( node );
	}

	return NULL;
}

Material *RM_CacheMaterial( const char *path, CacheGroup group ) {
	/* check if it's already cached */
	Material *material = RM_GetMaterial( path, group );
	if ( material != NULL ) {
		return material;
	}

	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		PrintWarn( "Failed to load material, \"%s\"!\nPL: %s\n", path, plGetError() );
		return fallbackMaterial;
	}

	material = RM_ParseMaterial( file );

	plCloseFile( file );

	if ( material == NULL ) {
		PrintWarn( "Failed to cache material, \"%s\"!\n", path );
		return fallbackMaterial;
	}

	snprintf( material->path, sizeof( material->path ), "%s", path );
	material->node = plInsertLinkedListNode( materials[ group ], material );

	return material;
}

void RM_DestroyMaterial( Material *material ) {
	PLLinkedList *container = plGetLinkedListNodeContainer( material->node );
	if ( container != NULL ) {
		plDestroyLinkedListNode( container, material->node );
	}

	free( material );
}

void RM_ClearMaterials( CacheGroup group ) {
	PLLinkedListNode *node = plGetRootNode( materials[ group ] );
	while ( node != NULL ) {
		free( plGetLinkedListNodeUserData( node ) );
		node = plGetNextLinkedListNode( node );
	}

	plDestroyLinkedListNodes( materials[ group ] );
}

static void RM_SetBuiltInVariable( PLShaderProgram *program, int uniformSlot, int variable ) {
	if ( variable == -1 ) {
		return;
	}

	switch( variable ) {
		case MATERIAL_BUILTIN_TIME: {
			unsigned int numTicks = Engine_GetNumTicks();
			plSetShaderUniformValueByIndex( program, uniformSlot, &numTicks, false );
			break;
		}

		default: break;
	}
}

void RM_DrawMesh( Material *material, PLMesh *mesh ) {
	for ( unsigned int i = 0; i < material->numPasses; ++i ) {
		MaterialPass *curPass = &material->passes[ i ];

		plSetShaderProgram( curPass->program );
		plSetBlendMode( curPass->blendMode[ 0 ], curPass->blendMode[ 1 ] );

		plSetShaderUniformValue( curPass->program, "pl_model", plGetMatrix( PL_MODELVIEW_MATRIX ), true );

		unsigned int curUnit = 0;
		for ( unsigned int j = 0; j < curPass->numVariables; ++j ) {
			/* textures just need to be set per their respective unit */
			if ( curPass->variables[ j ].varData.type == SCRIPT_VAR_TEXTURE ) {
				plSetTexture( curPass->variables[ j ].varData.value.texVar, curUnit );
				plSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curUnit, false );
				curUnit++;
				continue;
			}
			/* built-in variables are special cases */
			else if ( curPass->variables[ j ].varData.type == SCRIPT_VAR_BUILTIN ) {
				RM_SetBuiltInVariable( curPass->program, curPass->variables[ j ].programSlot, curPass->variables[ j ].varData.value.iVar );
				continue;
			}

			plSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curPass->variables[ j ].varData.value, false );
		}

		plUploadMesh( mesh );
		plDrawMesh( mesh );
	}
}
