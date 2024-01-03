// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Model editor tab
// Author:  Mark E. Sowden

#include "ModelEditor.h"

#include "../ViewportFrame.h"

FXDEFMAP( ss::forge::ModelEditor )
modelEditorMap[]={

};
FXIMPLEMENT(ss::forge::ModelEditor, FXTabItem, modelEditorMap, ARRAYNUMBER( modelEditorMap ) )

ss::forge::ModelEditor::ModelEditor( FXTabBook *owner, const FXString &worldName, SSApeModel *model ) : FXTabItem( owner, "Model Editor" )
{
	setIcon( ss::forge::load_fx_icon( getApp(), "resources/model_editor.gif" ) );

	auto *frame = new FXVerticalFrame( owner, LAYOUT_FILL );

	auto *toolbar = new FXToolBar( frame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/save.gif" ) );
	new FXVerticalSeparator( toolbar );
	_editModeButtons[ EDITOR_GEOMETRYMODE_BRUSH ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/brush_mode.gif" ), 0, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	_editModeButtons[ EDITOR_GEOMETRYMODE_VERTEX ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/vertex_mode.gif" ), 0, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	_editModeButtons[ EDITOR_GEOMETRYMODE_EDGE ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/edge_mode.gif" ), 0, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	_editModeButtons[ EDITOR_GEOMETRYMODE_FACE ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/face_mode.gif" ), 0, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	//editModeButtons[ currentEditMode ]->setState( true );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/grid.gif" ), 0, &_gridSizeTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/play.gif" ) );

	unsigned int mode = SS_ARL_CAMERA_MODE_PERSPECTIVE;
	auto *hs = new FX4Splitter( frame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	_viewport = new ViewportFrame( hs, get_shared_gl_visual(), ( SSArlCameraMode ) mode++ );

	frame->create();

	if ( !worldName.empty() )
	{
		setText( "Material Editor (" + worldName + ")" );
	}
}

ss::forge::ModelEditor::~ModelEditor() = default;
