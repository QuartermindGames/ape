/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#include <PL/pl_llist.h>

#include "yin.h"
#include "renderer.h"
#include "material.h"

static PLLinkedList *materials[ MAX_CACHE_GROUPS ];
//static PLLinkedList *shaderPrograms;

typedef struct MaterialVariable {
	char name[ PL_SYSTEM_MAX_PATH ];
	int programSlot;
	int type;
	union {
		float fVar;
		int iVar;
		unsigned int uiVar;
		bool bVar;
		double dVar;
		PLVector2 v2Var;
		PLVector3 v3Var;
		PLVector4 v4Var;
		PLTexture *texVar;
	} value;
} MaterialVariable;

typedef struct MaterialPass {
	PLShaderProgram *program;
	PLBlend blendMode[ 2 ];
	MaterialVariable variables[ MAX_MATERIAL_VARIABLES ];
	unsigned int numVariables;
} MaterialPass;

typedef struct Material {
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

static int RM_GetVariableTypeByTag( const char *tag ) {
	static const char *materialVarTags[ MAX_MATERIAL_VAR_TYPES ]={
			[ MATERIAL_VAR_FLOAT ] = "float",
			[ MATERIAL_VAR_INT ] = "int",
	        [ MATERIAL_VAR_UINT ] = "uint",
			[ MATERIAL_VAR_BOOL ] = "bool",
			[ MATERIAL_VAR_DOUBLE ] = "double",
			[ MATERIAL_VAR_VEC2 ] = "vec2",
			[ MATERIAL_VAR_VEC3 ] = "vec3",
			[ MATERIAL_VAR_VEC4 ] = "vec4",
			[ MATERIAL_VAR_TEXTURE ] = "texture",
	        [ MATERIAL_VAR_BUILTIN ] = "builtin",
	};

	for ( int i = 0; i < MAX_MATERIAL_VAR_TYPES; ++i ) {
		if ( strcmp( tag, materialVarTags[ i ] ) == 0 ) {
			return i;
		}
	}

	return -1;
}

static bool RM_ValidateVariableType( unsigned int materialType, PLShaderUniformType uniformType ) {
	static const PLShaderUniformType match[ MAX_MATERIAL_VAR_TYPES ] = {
		[ MATERIAL_VAR_FLOAT ] = PL_UNIFORM_FLOAT,
		[ MATERIAL_VAR_INT ] = PL_UNIFORM_INT,
	    [ MATERIAL_VAR_UINT ] = PL_UNIFORM_UINT,
		[ MATERIAL_VAR_BOOL ] = PL_UNIFORM_BOOL,
		[ MATERIAL_VAR_DOUBLE ] = PL_UNIFORM_DOUBLE,
		[ MATERIAL_VAR_VEC2 ] = PL_UNIFORM_VEC2,
		[ MATERIAL_VAR_VEC3 ] = PL_UNIFORM_VEC3,
		[ MATERIAL_VAR_VEC4 ] = PL_UNIFORM_VEC4,
		[ MATERIAL_VAR_TEXTURE ] = PL_UNIFORM_SAMPLER2D,
	};

	/* built-in variables are special */
	if ( materialType == MATERIAL_VAR_BUILTIN ) {
		return true;
	}

	if ( materialType >= MAX_MATERIAL_VAR_TYPES || match[ materialType ] != uniformType ) {
		return false;
	}

	return true;
}

static int RM_GetBlendModeByTag( const char *tag ) {
	static const char *blendModeTags[ PL_MAX_BLEND_MODES ] = {
			[ PL_BLEND_NONE ] = "none",
			[ PL_BLEND_ZERO ] = "zero",
			[ PL_BLEND_ONE ] = "one",
			[ PL_BLEND_SRC_COLOR ] = "src_color",
			[ PL_BLEND_ONE_MINUS_SRC_COLOR ] = "one_minus_src_color",
			[ PL_BLEND_SRC_ALPHA ] = "src_alpha",
			[ PL_BLEND_ONE_MINUS_SRC_ALPHA ] = "one_minus_src_alpha",
			[ PL_BLEND_DST_ALPHA ] = "dst_alpha",
			[ PL_BLEND_ONE_MINUS_DST_ALPHA ] = "one_minus_dst_alpha",
			[ PL_BLEND_DST_COLOR ] = "dst_color",
			[ PL_BLEND_ONE_MINUS_DST_COLOR ] = "one_minus_dst_color",
			[ PL_BLEND_SRC_ALPHA_SATURATE ] = "src_alpha_saturate",
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

static Material *RM_ParseMaterial( PLFile *file ) {
	char buffer[ 1024 ];

	/* first thing we should find is the identifier */
	plReadString( file, buffer, sizeof( buffer ) );
	if ( strcmp( "material\n", buffer ) != 0 ) {
		PrintWarn( "Invalid identifier for material!\n" );
		return NULL;
	}

	Material mat;

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
			continue;
		}

		/* blend mode */
		if ( strncmp( "blend ", r, 6 ) == 0 ) {
			char blendModeNames[ 2 ][ 32 ];
			if ( sscanf( r, "blend %s %s\n", blendModeNames[ 0 ], blendModeNames[ 1 ] ) != 2 ) {
				PrintWarn( "Failed to read in blend for pass %d\n", mat.numPasses + 1 );
				continue;
			}

			curPass->blendMode[ 0 ] = RM_GetBlendModeByTag( blendModeNames[ 0 ] );
			curPass->blendMode[ 1 ] = RM_GetBlendModeByTag( blendModeNames[ 1 ] );
			continue;
		}

		/* variable */
		if ( strncmp( "var ", r, 4 ) == 0 ) {
			if ( curPass == NULL ) {
				PrintWarn( "Pass not specified in program, skipping variable!\n" );
				continue;
			}

			/* get the variable type */
			char *token = strtok( r + 4, " " );
			if ( token == NULL ) {
				PrintWarn( "Failed to get variable type!\n" );
				continue;
			}

			unsigned int i = curPass->numVariables;
			curPass->variables[ i ].type = RM_GetVariableTypeByTag( token );
			if ( curPass->variables[ i ].type == -1 ) {
				PrintWarn( "Invalid variable type \"%s\"!\n", token );
				continue;
			}

			token = strtok( NULL, " " );
			if ( token == NULL ) {
				PrintWarn( "Failed to get variable name!\n" );
				continue;
			}

			/* copy it across */
			snprintf( curPass->variables[ i ].name, sizeof( curPass->variables[ i ].name ), "%s", token );

			curPass->variables[ i ].programSlot = plGetShaderUniformSlot( curPass->program, curPass->variables[ i ].name );
			if ( curPass->variables[ i ].programSlot == -1 ) {
				PrintWarn( "Failed to fetch uniform slot for variable \"%s\"!\n", curPass->variables[ i ].name );
				continue;
			}

			/* fetch the uniform type so we can validate it, urgh */
			unsigned int uniformType = plGetShaderUniformType( curPass->program, curPass->variables[ i ].programSlot );
			if ( !RM_ValidateVariableType( curPass->variables[ i ].type, uniformType ) ) {
				PrintWarn( "Material variable \"%s\" type does not match uniform type!\n", curPass->variables[ i ].name );
				continue;
			}

			/* and now, we need to read in the actual value */

			token = strtok( NULL, " \n" );
			if ( token == NULL ) {
				PrintWarn( "Failed to get variable \"%s\" value!\n", curPass->variables[ i ].name );
				continue;
			}

			/* handle built-in variables */


			switch( curPass->variables[ i ].type ) {
				case MATERIAL_VAR_BUILTIN:
					curPass->variables[ i ].value.iVar = RM_GetBuiltInByTag( token );
					if ( curPass->variables[ i ].value.iVar == -1 ) {
						PrintWarn( "Invalid built-in type \"%s\"!\n", token );
					}
					break;
				case MATERIAL_VAR_DOUBLE:
					curPass->variables[ i ].value.dVar = strtod( token, NULL );
					break;
				case MATERIAL_VAR_BOOL:
					if ( strcmp( token, "true" ) == 0 ) {
						curPass->variables[ i ].value.bVar = true;
					} else {
						curPass->variables[ i ].value.bVar = false;
					}
					break;
				case MATERIAL_VAR_FLOAT:
					curPass->variables[ i ].value.fVar = strtof( token, NULL );
					break;
				case MATERIAL_VAR_INT:
					curPass->variables[ i ].value.iVar = strtol( token, NULL, 10 );
					break;
				case MATERIAL_VAR_UINT:
					curPass->variables[ i ].value.uiVar = strtoul( token, NULL, 10 );
					break;
				case MATERIAL_VAR_TEXTURE:
					curPass->variables[ i ].value.texVar = Gfx_LoadTexture( token );
					break;
			}

			curPass->numVariables++;
			continue;
		}

		if ( strcmp( "end\n", r ) == 0 ) {
			break;
		}

		r = plReadString( file, buffer, sizeof( buffer ) );
	}

	Material *out = g_system.malloc( sizeof( Material ) );
	memcpy( out, &mat, sizeof( Material ) );
	return out;
}

Material *RM_CacheMaterial( const char *path, CacheGroup group ) {
	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		PrintWarn( "Failed to load material, \"%s\"!\nPL: %s\n", path, plGetError() );
		return NULL;
	}

	Material *material = RM_ParseMaterial( file );

	plCloseFile( file );

	if ( material == NULL ) {
		return NULL;
	}

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
			if ( curPass->variables[ j ].type == MATERIAL_VAR_TEXTURE ) {
				plSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curUnit, false );
				plSetTexture( curPass->variables[ j ].value.texVar, curUnit++ );
				continue;
			}
			/* built-in variables are special cases */
			else if ( curPass->variables[ j ].type == MATERIAL_VAR_BUILTIN ) {
				RM_SetBuiltInVariable( curPass->program, curPass->variables[ j ].programSlot, curPass->variables[ j ].value.iVar );
				continue;
			}

			plSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curPass->variables[ j ].value, false );
		}

		plUploadMesh( mesh );
		plDrawMesh( mesh );
	}
}
