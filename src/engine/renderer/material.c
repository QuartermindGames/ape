/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#include <PL/pl_llist.h>

#include "yin.h"
#include "material.h"

static PLLinkedList *materials;

typedef struct MaterialVariable {
	char *name;
	unsigned int programSlot;
	unsigned int type;
	union {
		float fVar;
		int iVar;
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
	MaterialVariable variables[ MATERIAL_MAX_VARIABLES ];
	unsigned int numVariables;
} MaterialPass;

typedef struct Material {
	MaterialPass passes[ MATERIAL_MAX_PASSES ];
	unsigned int numPasses;
	PLLinkedListNode *node;
} Material;

void RM_InitializeMaterialSystem( void ) {
	PrintMsg( "Initializing material system\n" );

	materials = plCreateLinkedList();
}

void RM_ShutdownMaterialSystem( void ) {
	RM_ClearMaterials();

	plDestroyLinkedList( materials );
}

PLShaderProgram *RM_GetMaterialShaderProgram( Material *material ) {
	return NULL;
}

static Material *RM_ParseMaterial( PLFile *file ) {

}

void RM_CacheMaterial( const char *path ) {
	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		PrintWarn( "Failed to load material, \"%s\"!\nPL: %s\n", path, plGetError() );
		return;
	}

	Material *material = RM_ParseMaterial( file );

	plCloseFile( file );

	if ( material == NULL ) {
		return;
	}

	material->node = plInsertLinkedListNode( materials, material );
}

void RM_ClearMaterials( void ) {

}
