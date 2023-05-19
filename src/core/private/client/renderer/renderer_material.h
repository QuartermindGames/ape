// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#define MAX_MATERIAL_PASSES    4
#define MAX_MATERIAL_VARIABLES 64

/* built-in variable types */
typedef enum OGEMaterialBuiltinVar
{
	MATERIAL_BUILTIN_INVALID = -1,
	MATERIAL_BUILTIN_TIME,
	MATERIAL_BUILTIN_DEPTH,
	MATERIAL_BUILTIN_VIEWPORT_SIZE,

	MAX_MATERIAL_BUILTINS
} OGEMaterialBuiltinVar;

typedef struct YNCoreMaterial YNCoreMaterial;

#define MATERIAL_VAR_NAME_LENGTH   64
#define MATERIAL_VAR_STRING_LENGTH 256

typedef enum OGEMaterialVariableType
{
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
} OGEMaterialVariableType;

/**
 * Hints for standard material variables, so
 * that we can toggle their state.
 */
typedef enum OGEMaterialVariableHint
{
	RM_VAR_HINT_DIFFUSE,
	RM_VAR_HINT_NORMAL,
	RM_VAR_HINT_SPECULAR,
} OGEMaterialVariableHint;

typedef union OGEMaterialVariableData
{
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
} OGEMaterialVariableData;

typedef struct OGEMaterialVariable
{
	int programSlot;
	char name[ MATERIAL_VAR_NAME_LENGTH ];
	OGEMaterialVariableType type;
	OGEMaterialVariableData data;
	OGEMaterialVariableHint hint;
} OGEMaterialVariable;

typedef struct OGEMaterialPass
{
	PLGShaderProgram *program;
	PLGTextureFilter textureFilter;
	PLGBlend blendMode[ 2 ];
	OGEMaterialVariable variables[ MAX_MATERIAL_VARIABLES ];
	unsigned int numVariables;

	bool depthTest;
	int cullMode;
} OGEMaterialPass;

#define RS_PROGRAM_NAME_LENGTH 64

typedef enum OGEDefaultShaderProgram
{
	OGE_SHADER_DEFAULT,
	OGE_SHADER_LIGHTING_PASS,
	OGE_SHADER_DEFAULT_VERTEX,
	OGE_SHADER_DEFAULT_ALPHA,
	OGE_SHADER_DEFAULT_FONT,

	OGE_MAX_DEFAULT_SHADERS
} OGEDefaultShaderProgram;
extern PLGShaderProgram *oge_defaultShaderPrograms[ OGE_MAX_DEFAULT_SHADERS ];

typedef struct OGEShaderProgramIndex
{
	char path[ PL_SYSTEM_MAX_PATH ];
	char shaderPaths[ PLG_MAX_SHADER_TYPES ][ PL_SYSTEM_MAX_PATH ];
	char internalName[ RS_PROGRAM_NAME_LENGTH ];

	OGEMaterialPass defaultPass;

	PLGShaderProgram *internalPtr;
	struct PLLinkedListNode *node;
} OGEShaderProgramIndex;

void ogeMaterial_ParsePass( struct YNNodeBranch *root, OGEMaterialPass *materialPass );

void ogeInitializeMaterialSystem( void );
void ogeShutdownMaterialSystem( void );

PLGTexture *YnCore_Material_GetPreviewTexture( YNCoreMaterial *material );

YNCoreMaterial *YnCore_GetFallbackMaterial( void );
