// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#define MAX_MATERIAL_PASSES    4
#define MAX_MATERIAL_VARIABLES 64

/* built-in variable types */
typedef enum ApeMaterialBuiltinVar {
	APE_MATERIAL_BUILTIN_INVALID = -1,
	APE_MATERIAL_BUILTIN_TIME,
	APE_MATERIAL_BUILTIN_DEPTH,
	APE_MATERIAL_BUILTIN_VIEWPORT_SIZE,
	APE_MATERIAL_BUILTIN_FALLBACK,// todo: replace with 'proc', and determine proc type

	APE_MAX_MATERIAL_BUILTINS
} ApeMaterialBuiltinVar;

typedef struct ApeMaterial ApeMaterial;

#define MATERIAL_VAR_NAME_LENGTH   64
#define MATERIAL_VAR_STRING_LENGTH 256

typedef enum ApeMaterialVariableType {
	MATERIAL_VAR_INVALID,

	MATERIAL_VAR_FLOAT,
	MATERIAL_VAR_INT,
	MATERIAL_VAR_UINT,
	MATERIAL_VAR_BOOL,
	MATERIAL_VAR_DOUBLE,

	MATERIAL_VAR_VEC2,
	MATERIAL_VAR_VEC3,
	MATERIAL_VAR_VEC4,

	MATERIAL_VAR_MAT3,
	MATERIAL_VAR_MAT4,

	/* special types */
	MATERIAL_VAR_STRING,
	MATERIAL_VAR_TEXTURE,
	MATERIAL_VAR_BUILTIN,
	MATERIAL_VAR_RENDERTARGET,

	MAX_MATERIAL_VAR_TYPES
} ApeMaterialVariableType;

/**
 * Hints for standard material variables, so
 * that we can toggle their state.
 */
typedef enum ApeMaterialVariableHint {
	APE_MAT_VAR_HINT_DIFFUSE,
	APE_MAT_VAR_HINT_NORMAL,
	APE_MAT_VAR_HINT_SPECULAR,
} ApeMaterialVariableHint;

typedef union ApeMaterialVariableData {
	float f32;
	double f64;

	bool boolean;

	int32_t i32;
	uint32_t ui32;

	PLVector2 vec2;
	PLVector3 vec3;
	PLVector4 vec4;

	PLMatrix3 mat3;
	PLMatrix4 mat4;

	char str[ MATERIAL_VAR_STRING_LENGTH ];

	void *userPtr;
} ApeMaterialVariableData;

typedef struct ApeMaterialVariable {
	int programSlot;
	char name[ MATERIAL_VAR_NAME_LENGTH ];
	ApeMaterialVariableType type;
	ApeMaterialVariableData data;
	ApeMaterialVariableHint hint;
} ApeMaterialVariable;

typedef struct ApeMaterialPass {
	PLGShaderProgram *program;
	PLGTextureFilter textureFilter;
	PLGBlend blendMode[ 2 ];
	ApeMaterialVariable variables[ MAX_MATERIAL_VARIABLES ];
	unsigned int numVariables;

	bool depthTest;
	int cullMode;
} ApeMaterialPass;

#define RS_PROGRAM_NAME_LENGTH 64

extern PLGShaderProgram *ape_defaultShaderPrograms_[ APE_MAX_DEFAULT_SHADERS ];

typedef struct ApeShaderProgramIndex {
	char path[ PL_SYSTEM_MAX_PATH ];
	char shaderPaths[ PLG_MAX_SHADER_TYPES ][ PL_SYSTEM_MAX_PATH ];
	char internalName[ RS_PROGRAM_NAME_LENGTH ];

	ApeMaterialPass defaultPass;

	PLGShaderProgram *internalPtr;
	struct PLLinkedListNode *node;
} ApeShaderProgramIndex;

PL_EXTERN_C

void apeParseMaterialPass( struct NdBranch *root, ApeMaterialPass *materialPass );

void apeInitializeMaterialSystem( void );
void apeShutdownMaterialSystem( void );

ApeMaterial *apeGetFallbackMaterial( void );
ApeMaterial *apeGetVertexMaterial( void );

PL_EXTERN_C_END
