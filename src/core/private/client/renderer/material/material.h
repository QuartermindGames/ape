// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

typedef struct ApeShaderProgram ApeShaderProgram;
typedef struct ApeMaterial ApeMaterial;

#define SS_ARL_MAX_MATERIAL_PASSES    4
#define SS_ARL_MAX_MATERIAL_VARIABLES 64

/* built-in variable types */
typedef enum ApeMaterialBuiltinVar
{
	SS_ARL_MATERIAL_BUILTIN_INVALID = -1,
	SS_ARL_MATERIAL_BUILTIN_TIME,
	SS_ARL_MATERIAL_BUILTIN_DEPTH,
	SS_ARL_MATERIAL_BUILTIN_VIEWPORT_SIZE,
	SS_ARL_MATERIAL_BUILTIN_FALLBACK,// todo: replace with 'proc', and determine proc type

	SS_ARL_MAX_MATERIAL_BUILTINS
} ApeMaterialBuiltinVar;

typedef enum ApeMaterialFlag
{
	PL_BITFLAG( APE_MATERIAL_FLAG_MIRROR, 0U ),
	PL_BITFLAG( APE_MATERIAL_FLAG_SHADOWS, 1U ),
	PL_BITFLAG( APE_MATERIAL_FLAG_BLENDED, 2U ),
} ApeMaterialFlag;

#define APE_MATERIAL_VAR_NAME_LENGTH 64

typedef enum ApeMaterialVariableType
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
} ApeMaterialVariableType;

/**
 * Hints for standard material variables, so
 * that we can toggle their state.
 */
typedef enum ApeMaterialVariableHint
{
	SS_ARL_MATERIAL_VAR_HINT_DIFFUSE,
	SS_ARL_MATERIAL_VAR_HINT_NORMAL,
	SS_ARL_MATERIAL_VAR_HINT_SPECULAR,
} ApeMaterialVariableHint;

typedef union ApeMaterialVariableData
{
	ApeMaterialBuiltinVar builtinVar;
	void *ptr;
} ApeMaterialVariableData;

typedef struct ApeMaterialVariable
{
	int programSlot;
	char name[ APE_MATERIAL_VAR_NAME_LENGTH ];

	ApeMaterialVariableType type;// type of data
	ApeMaterialVariableData data;// data store
	unsigned int numElements;    // number of elements (i.e., is it an array?)

	ApeMaterialVariableHint hint;
} ApeMaterialVariable;

typedef struct ApeMaterialPass
{
	ApeShaderProgram *program;

	PLGTextureFilter textureFilter;
	PLVector2 textureScroll;
	PLVector2 textureOffset;

	PLGBlend blendMode[ 2 ];
	ApeMaterialVariable variables[ SS_ARL_MAX_MATERIAL_VARIABLES ];
	unsigned int numVariables;

	bool depthTest;
	int cullMode;
} ApeMaterialPass;

/////////////////////////////////////////////////////////////////////////////////////
// Shaders

#define RS_PROGRAM_NAME_LENGTH 64

typedef struct ApeShaderProgram
{
	char internalName[ RS_PROGRAM_NAME_LENGTH ];

	PLPath path;
	time_t timestamp;

	PLPath sourcePaths[ PLG_MAX_SHADER_TYPES ];
	time_t sourceTimestamps[ PLG_MAX_SHADER_TYPES ];

	ApeMaterialPass defaultPass;

	PLGShaderProgram *internal;
} ApeShaderProgram;

void ape_hot_reload_shaders_();

ApeShaderProgram *ape_get_shader_by_name( const char *name );

void ape_set_active_shader_by_default_( ApeDefaultShaderProgram defaultShaderProgram );

void ape_shader_set_active_( ApeShaderProgram *self );

/////////////////////////////////////////////////////////////////////////////////////

void ape_parse_material_pass_( struct NdBranch *root, ApeMaterialPass *materialPass );

void ape_initialize_materials_( void );
void ape_shutdown_materials_( void );

PLGTexture *ape_material_get_texture_( ApeMaterial *self, unsigned int pass, const char *hint );

bool ape_material_shadows_enabled( const ApeMaterial *self );
bool ape_material_is_blended( const ApeMaterial *self );

void ape_tick_materials_( void );

PL_EXTERN_C_END
