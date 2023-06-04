// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "common.h"
#include "core_world.h"

PL_EXTERN_C

typedef enum ApeEditorContextType
{
	YN_CORE_EDITOR_CONTEXT_WORLD,
	//YN_CORE_EDITOR_CONTEXT_MODEL,
	//YN_CORE_EDITOR_CONTEXT_MATERIAL,

	YN_CORE_EDITOR_MAX_CONTEXTS
} ApeEditorContextType;

#define APE_EDITOR_MAX_VIEWPORTS      4
#define APE_EDITOR_MAX_VIEW_BOOKMARKS 16

typedef struct ApeEditorField
{
	char name[ 64 ];
	char description[ 128 ];
	CmnDataType type;
	uintptr_t varOffset;
} ApeEditorField;

#define APE_ENTITY_COMPONENT_BEGIN_PROPERTIES() static ApeEditorField x_editorVariables[] = {
#define APE_ENTITY_COMPONENT_END_PROPERTIES() \
	}                                         \
	;                                         \
	static unsigned int x_numEditorVariables = PL_ARRAY_ELEMENTS( x_editorVariables );
#define APE_ENTITY_COMPONENT_PROPERTY( TYPE, VAR, DESC, VARTYPE ) \
	{ #VAR, DESC, VARTYPE, PL_OFFSETOF( TYPE, VAR ) },
#define APE_ENTITY_HOOK_PROPERTIES( CBTABLE )        \
	( CBTABLE ).editorFields    = x_editorVariables; \
	( CBTABLE ).numEditorFields = x_numEditorVariables

typedef struct ApeEditorViewBookmark
{
	char description[ 32 ];
	PLVector3 viewPos;
	PLVector3 viewAngles;
} ApeEditorViewBookmark;

typedef enum ApeEditorGeometryMode
{
	EDITOR_GEOMETRYMODE_VERTEX,
	EDITOR_GEOMETRYMODE_EDGE,
	EDITOR_GEOMETRYMODE_FACE,
	EDITOR_GEOMETRYMODE_BRUSH,

	EDITOR_MAX_GEOMETRYMODES
} ApeEditorGeometryMode;

typedef struct ApeEditorGlobalContext
{
	ApeWorld *world;
} ApeEditorGlobalContext;
ApeEditorGlobalContext *YnCore_GetGlobalEditorContext( void );

/* An instance represents literally a unique instance
 * of an editing state, which can have it's own world, model
 * or whatever resource open.
 * */
typedef struct ApeEditorContext
{
	const char *name, *identifier;

	// required
	void ( *RegisterConsoleVariables )( void );
	void ( *Initialize )( void );
	void ( *Shutdown )( void );
	void ( *Draw )( void );
	void ( *DrawGUI )( void );
	void ( *Tick )( void );
	// optionals
	void ( *OnActive )( void );

	ApeEditorContextType mode;

	bool hideGrid;
	bool useLineGrid;
	PLMatrix4 gridTransform;
	int32_t gridScale;

	ApeViewport *viewports[ APE_EDITOR_MAX_VIEWPORTS ];
	ApeCamera *camera;
} ApeEditorContext;

PL_EXTERN_C_END
