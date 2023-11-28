// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "common.h"
#include "core_world.h"

PL_EXTERN_C

#define APE_EDITOR_MAX_VIEWPORTS      4
#define APE_EDITOR_MAX_VIEW_BOOKMARKS 16

typedef struct SSAclEditorField
{
	char name[ 64 ];
	char description[ 128 ];
	ComDataType type;
	uintptr_t varOffset;
} SSAclEditorField;

#define APE_ENTITY_COMPONENT_BEGIN_PROPERTIES() static SSAclEditorField x_editorVariables[] = {
#define APE_ENTITY_COMPONENT_END_PROPERTIES() \
	}                                         \
	;                                         \
	static unsigned int x_numEditorVariables = PL_ARRAY_ELEMENTS( x_editorVariables );
#define APE_ENTITY_COMPONENT_PROPERTY( TYPE, VAR, DESC, VARTYPE ) \
	{ #VAR, DESC, VARTYPE, PL_OFFSETOF( TYPE, VAR ) },
#define APE_ENTITY_HOOK_PROPERTIES( CBTABLE )     \
	( CBTABLE ).editorFields = x_editorVariables; \
	( CBTABLE ).numEditorFields = x_numEditorVariables

typedef enum SSAclEditorGeometryMode
{
	EDITOR_GEOMETRYMODE_VERTEX,
	EDITOR_GEOMETRYMODE_EDGE,
	EDITOR_GEOMETRYMODE_FACE,
	EDITOR_GEOMETRYMODE_BRUSH,

	EDITOR_MAX_GEOMETRYMODES
} SSAclEditorGeometryMode;

void ss_acl_increase_grid_size( void );
void ss_acl_decrease_grid_size( void );

PL_EXTERN_C_END
