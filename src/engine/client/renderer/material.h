/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#define MAX_MATERIAL_PASSES	   4
#define MAX_MATERIAL_VARIABLES 64

/* built-in variable types */
enum
{
	MATERIAL_BUILTIN_TIME,

	MAX_MATERIAL_BUILTINS
};

typedef struct Material Material;

void RM_InitializeMaterialSystem( void );
void RM_ShutdownMaterialSystem( void );

PLGShaderProgram *RM_GetMaterialShaderProgram( Material *material, unsigned int pass );

/* i/o */
Material *RM_CacheMaterial( const char *path, CacheGroup group, bool useFallback );
void	  RM_ReleaseMaterial( Material *material );

/* drawing */
void RM_DrawMesh( Material *material, PLGMesh *mesh );

Material *RM_GetFallbackMaterial( void );
