// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Editor interface.
// Author:  Mark E. Sowden

#include "ape_private.h"
#include "editor.h"

static ApeMaterial *uiMaterial;

typedef struct UiWidget UiWidget;
typedef struct UiButton UiButton;

typedef enum UiClassType : uint8_t
{
	UI_CLASS_TYPE_PANEL,
	UI_CLASS_TYPE_BUTTON,
} UiClassType;

typedef struct UiPanel
{
} UiPanel;

typedef struct UiButton
{
	void ( *clickCallback )( UiButton *button, void *usr );
} UiButton;

typedef struct UiWidget
{
	UiWidget      *parent;
	QmMathVector2i position;// relative to parent
	QmMathVector2i size;
	UiClassType    type;
	union
	{
		UiButton button;
	};
} UiWidget;

/////////////////////////////////////////////////////////////////////////////////////
// Toolbar
// The toolbar is basically a movable bar that includes various widgets/modes you can
// pick from.
/////////////////////////////////////////////////////////////////////////////////////

static struct
{
	QmMathVector2i position;
	QmMathVector2i size;
} uiToolbar;

static void ui_toolbar_draw( const ApeViewport *viewport )
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void ape_editor_ui_initialize_()
{
	uiMaterial = ape_material_cache( "materials/editor/editor_ui.mat.n", APE_CACHE_GROUP_EDITOR, true );
}

void ape_editor_ui_shutdown_()
{
	ape_material_release_reference( uiMaterial );
	uiMaterial = nullptr;
}

void ape_editor_ui_draw_( const ApeViewport *viewport )
{
}

void ape_editor_ui_tick_( const ApeViewport *viewport, double delta )
{
}
