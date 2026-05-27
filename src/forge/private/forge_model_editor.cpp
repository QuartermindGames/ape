// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Model editor tab
// Author:  Mark E. Sowden

#include "forge_model_editor.h"
#include "forge_model_viewport.h"

#include "yin/core_entity.h"
#include "ape/ape_public_model.h"

FXDEFMAP( forge::editor_model )
modelEditorMap[] = {

};
FXIMPLEMENT( forge::editor_model, forge::EditorTab, modelEditorMap, ARRAYNUMBER( modelEditorMap ) )

forge::editor_model::editor_model( FXTabBook *owner, const FXString &modelName, ApeModel *model ) : forge::EditorTab( owner, "Model Editor", nullptr, APE_EDITOR_MODE_MODEL )
{
	setIcon( forge::load_fx_icon( getApp(), "resources/model.gif" ) );

	auto *frame = new FXVerticalFrame( owner, LAYOUT_FILL );

	auto *toolbar = new FXToolBar( frame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/save.gif" ) );
	new FXVerticalSeparator( toolbar );
	_editModeButtons[ APE_EDITOR_GEOMETRY_MODE_VERTEX ] = new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/vertex_mode.gif" ), 0, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	//_editModeButtons[ APE_EDITOR_GEOMETRY_MODE_EDGE ]   = new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/edge_mode.gif" ), 0, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	_editModeButtons[ APE_EDITOR_GEOMETRY_MODE_FACE ]   = new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/face_mode.gif" ), 0, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	//editModeButtons[ currentEditMode ]->setState( true );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/grid.gif" ), 0, &_gridSizeTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/play.gif" ) );

	unsigned int mode = APE_CAMERA_MODE_PERSPECTIVE;
	auto        *hs   = new FX4Splitter( frame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	_viewport         = new Viewport( hs, get_shared_gl_visual(), nullptr, ( ApeCameraViewMode ) mode++ );

	frame->create();

	if ( !modelName.empty() )
	{
		setText( "Material Editor (" + modelName + ")" );
	}

	this->model = model;
}

forge::editor_model::~editor_model()
{
	ape_model_release_reference( model );

	ape_world_node_destroy( &world->base );
}
