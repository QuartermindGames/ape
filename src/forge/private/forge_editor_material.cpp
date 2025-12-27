// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Material editor tab
// Author:  Mark E. Sowden

#include "forge_editor_material.h"
#include "forge_viewport.h"

FXDEFMAP( forge::MaterialEditor )
materialEditorMap[] = {

};
FXIMPLEMENT( forge::MaterialEditor, FXTabItem, materialEditorMap, ARRAYNUMBER( materialEditorMap ) )

forge::MaterialEditor::MaterialEditor( FXTabBook *owner, const FXString &worldName, ApeMaterial *material ) : FXTabItem( owner, "Material Editor" )
{
	setIcon( forge::load_fx_icon( getApp(), "resources/material_editor.gif" ) );

	auto *frame = new FXVerticalFrame( owner, LAYOUT_FILL );

	auto *toolbar = new FXToolBar( frame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/save.gif" ) );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/grid.gif" ), 0, &_gridSizeTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/play.gif" ) );

	unsigned int mode = APE_CAMERA_MODE_PERSPECTIVE;
	auto        *hs   = new FX4Splitter( frame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	_viewport         = new Viewport( hs, get_shared_gl_visual(), nullptr, ( ApeCameraViewMode ) mode++ );

	frame->create();

	if ( !worldName.empty() )
	{
		setText( "Material Editor (" + worldName + ")" );
	}
}

forge::MaterialEditor::~MaterialEditor() = default;
