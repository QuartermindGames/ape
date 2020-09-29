/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#pragma once

#define MAX_MATERIAL_PASSES 4
#define MAX_MATERIAL_VARIABLES 64

/* built-in variable types */
enum {
	MATERIAL_BUILTIN_TIME,

	MAX_MATERIAL_BUILTINS
};

typedef struct Material Material;

void RM_InitializeMaterialSystem( void );
void RM_ShutdownMaterialSystem( void );

PLShaderProgram *RM_GetMaterialShaderProgram( Material *material, unsigned int pass );

/* i/o */
Material *RM_CacheMaterial( const char *path, CacheGroup group );
void RM_DestroyMaterial( Material *material );
void RM_ClearMaterials( CacheGroup group );

/* drawing */
void RM_SetMaterial( Material *material );
void RM_DrawMesh( Material *material, PLMesh *mesh );
