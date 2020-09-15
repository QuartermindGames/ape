/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#pragma once

#define MATERIAL_MAX_PASSES 4
#define MATERIAL_MAX_VARIABLES 64

enum {
	MATERIAL_VAR_FLOAT,
	MATERIAL_VAR_INT,
	MATERIAL_VAR_BOOL,
	MATERIAL_VAR_DOUBLE,

	MATERIAL_VAR_VEC2,
	MATERIAL_VAR_VEC3,
	MATERIAL_VAR_VEC4,

	MATERIAL_VAR_TEXTURE,
};

typedef struct Material Material;

void RM_InitializeMaterialSystem( void );
void RM_ShutdownMaterialSystem( void );

PLShaderProgram *RM_GetMaterialShaderProgram( Material *material );

/* i/o */
void RM_CacheMaterial( const char *path );
void RM_DestroyMaterial( Material *material );
void RM_ClearMaterials( void );

/* drawing */
void RM_SetMaterial( Material *material );
