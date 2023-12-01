// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#define SS_ARL_MAX_MATERIAL_PASSES    4
#define SS_ARL_MAX_MATERIAL_VARIABLES 64

/* built-in variable types */
typedef enum SS_Arl_MaterialBuiltinVar
{
	SS_ARL_MATERIAL_BUILTIN_INVALID = -1,
	SS_ARL_MATERIAL_BUILTIN_TIME,
	SS_ARL_MATERIAL_BUILTIN_DEPTH,
	SS_ARL_MATERIAL_BUILTIN_VIEWPORT_SIZE,
	SS_ARL_MATERIAL_BUILTIN_FALLBACK,// todo: replace with 'proc', and determine proc type

	SS_ARL_MAX_MATERIAL_BUILTINS
} SS_Arl_MaterialBuiltinVar;

typedef struct ApeMaterial ApeMaterial;

#define SS_ARL_MATERIAL_VAR_NAME_LENGTH   64
#define SS_ARL_MATERIAL_VAR_STRING_LENGTH 256

typedef enum SS_Arl_MaterialVariableType
{
	SS_ARL_MATERIAL_VAR_INVALID,

	SS_ARL_MATERIAL_VAR_FLOAT,
	SS_ARL_MATERIAL_VAR_INT,
	SS_ARL_MATERIAL_VAR_UINT,
	SS_ARL_MATERIAL_VAR_BOOL,
	SS_ARL_MATERIAL_VAR_DOUBLE,

	SS_ARL_MATERIAL_VAR_VEC2,
	SS_ARL_MATERIAL_VAR_VEC3,
	SS_ARL_MATERIAL_VAR_VEC4,

	SS_ARL_MATERIAL_VAR_MAT3,
	SS_ARL_MATERIAL_VAR_MAT4,

	/* special types */
	SS_ARL_MATERIAL_VAR_STRING,
	SS_ARL_MATERIAL_VAR_TEXTURE,
	SS_ARL_MATERIAL_VAR_BUILTIN,
	SS_ARL_MATERIAL_VAR_RENDERTARGET,

	SS_ARL_MAX_MATERIAL_VAR_TYPES
} SS_Arl_MaterialVariableType;

/**
 * Hints for standard material variables, so
 * that we can toggle their state.
 */
typedef enum SS_Arl_MaterialVariableHint
{
	SS_ARL_MATERIAL_VAR_HINT_DIFFUSE,
	SS_ARL_MATERIAL_VAR_HINT_NORMAL,
	SS_ARL_MATERIAL_VAR_HINT_SPECULAR,
} SS_Arl_MaterialVariableHint;

typedef union SS_Arl_MaterialVariableData
{
	SS_Arl_MaterialBuiltinVar builtinVar;
	void *ptr;
} SS_Arl_MaterialVariableData;

typedef struct SS_Arl_MaterialVariable
{
	int programSlot;
	char name[ SS_ARL_MATERIAL_VAR_NAME_LENGTH ];

	SS_Arl_MaterialVariableType type;// type of data
	SS_Arl_MaterialVariableData data;// data store
	unsigned int numElements;        // number of elements (i.e., is it an array?)

	SS_Arl_MaterialVariableHint hint;
} SS_Arl_MaterialVariable;

typedef struct SS_Arl_MaterialPass
{
	PLGShaderProgram *program;

	PLGTextureFilter textureFilter;
	PLVector2 textureScroll;
	PLVector2 textureOffset;

	PLGBlend blendMode[ 2 ];
	SS_Arl_MaterialVariable variables[ SS_ARL_MAX_MATERIAL_VARIABLES ];
	unsigned int numVariables;

	bool depthTest;
	int cullMode;
} SS_Arl_MaterialPass;

#define RS_PROGRAM_NAME_LENGTH 64

extern PLGShaderProgram *ape_defaultShaderPrograms_[ APE_MAX_DEFAULT_SHADERS ];

typedef struct SS_Arl_ShaderProgramIndex
{
	char path[ PL_SYSTEM_MAX_PATH ];
	char shaderPaths[ PLG_MAX_SHADER_TYPES ][ PL_SYSTEM_MAX_PATH ];
	char internalName[ RS_PROGRAM_NAME_LENGTH ];

	SS_Arl_MaterialPass defaultPass;

	PLGShaderProgram *internal;
	struct PLLinkedListNode *node;
} SS_Arl_ShaderProgramIndex;

PL_EXTERN_C

void ss_arl_material_parse_pass_( struct NdBranch *root, SS_Arl_MaterialPass *materialPass );

void ss_arl_initialize_materials_( void );
void ss_arl_shutdown_materials_( void );

typedef enum SSArlDefaultMaterial
{
	SS_ARL_MATERIAL_DEFAULT_FALLBACK,
	SS_ARL_MATERIAL_DEFAULT_VERTEX,
	SS_ARL_MATERIAL_DEFAULT_SHADOW,
	SS_ARL_MATERIAL_DEFAULT_DEPTH,

	SS_ARL_MAX_DEFAULT_MATERIALS
} SSArlDefaultMaterial;

ApeMaterial *ss_arl_get_default_material( SSArlDefaultMaterial defaultMaterial );

PLGTexture *ss_arl_material_get_texture_( ApeMaterial *material, unsigned int pass, const char *hint );

bool ss_arl_material_shadows_enabled( const ApeMaterial *material );

void ss_arl_tick_materials_( void );

PL_EXTERN_C_END
