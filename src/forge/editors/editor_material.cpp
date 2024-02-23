// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Material editor tab
// Author:  Mark E. Sowden

#include "editor_material.h"

#include "../forge_viewport_frame.h"

FXDEFMAP( ss::forge::editor_material )
materialEditorMap[] = {

};
FXIMPLEMENT( ss::forge::editor_material, FXTabItem, materialEditorMap, ARRAYNUMBER( materialEditorMap ) )

ss::forge::editor_material::editor_material( FXTabBook *owner, const FXString &worldName, ApeMaterial *material ) : FXTabItem( owner, "Material Editor" )
{
	setIcon( ss::forge::load_fx_icon( getApp(), "resources/material_editor.gif" ) );

	auto *frame = new FXVerticalFrame( owner, LAYOUT_FILL );

	auto *toolbar = new FXToolBar( frame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/save.gif" ) );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/grid.gif" ), 0, &_gridSizeTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/play.gif" ) );

	unsigned int mode = APE_CAMERA_MODE_PERSPECTIVE;
	auto *hs = new FX4Splitter( frame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	_viewport = new viewport_frame( hs, get_shared_gl_visual(), nullptr, ( ApeCameraViewMode ) mode++ );

	frame->create();

	if ( !worldName.empty() )
	{
		setText( "Material Editor (" + worldName + ")" );
	}
}

ss::forge::editor_material::~editor_material() = default;
