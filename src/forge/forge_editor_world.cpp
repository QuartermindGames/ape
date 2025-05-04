// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: World editor tab
// Author:  Mark E. Sowden

#include <unordered_map>

#include "forge_editor_world.h"
#include "forge/forge_viewport_world.h"
#include "common_project.h"
#include "forge_window_main.h"

/////////////////////////////////////////////////////////////////////////////////////
// Surface Inspector
/////////////////////////////////////////////////////////////////////////////////////

namespace forge
{
	class SurfaceInspector : public FXDialogBox
	{
		FXDECLARE( SurfaceInspector )

		ApeBrushFace *face;

		PLImage *preview{};
		FXLabel *previewIcon;

		FXTextField *tagField;

		FXTextField *scaleFieldX;
		FXTextField *scaleFieldY;

		FXTextField *offsetFieldX;
		FXTextField *offsetFieldY;

		FXTextField *rotationField;

	protected:
		SurfaceInspector() = default;

	public:
		enum
		{
			ID_SURFACE_PROPERTY = ID_LAST,
		};

		explicit SurfaceInspector( FXWindow *parent ) : FXDialogBox( parent, "Surface Inspector", DECOR_TITLE | DECOR_CLOSE | DECOR_BORDER | DECOR_MENU )
		{
			setWidth( 256 );
			//setHeight( 512 );

			setPadLeft( 0 );
			setPadRight( 0 );
			setPadBottom( 0 );
			setPadTop( 0 );

			FXVerticalFrame *vf = new FXVerticalFrame( this, LAYOUT_FILL );

			FXHorizontalFrame *hf;

#if 0
			hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X | LAYOUT_CENTER_X );
			previewIcon = new FXLabel( hf, FXString::null, nullptr, 0, LAYOUT_CENTER_X );
			previewIcon->setWidth( 64 );
			previewIcon->setHeight( 64 );

			new FXHorizontalSeparator( vf, SEPARATOR_GROOVE | LAYOUT_FILL_X );
#endif

			hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
			new FXLabel( hf, "Tag", nullptr, 0, LAYOUT_FILL_X );
			tagField = new FXTextField( hf, 4, nullptr, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | LAYOUT_FILL_X );

			new FXHorizontalSeparator( vf, SEPARATOR_GROOVE | LAYOUT_FILL_X );

			hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
			new FXLabel( hf, "Scale", nullptr, 0, LAYOUT_FILL_X );
			scaleFieldX = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );
			scaleFieldY = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );

			hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
			new FXLabel( hf, "Offset", nullptr, 0, LAYOUT_FILL_X );
			offsetFieldX = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );
			offsetFieldY = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );

			hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
			new FXLabel( hf, "Rotation", nullptr, 0, LAYOUT_FILL_X );
			rotationField = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );

			new FXHorizontalSeparator( vf, SEPARATOR_GROOVE | LAYOUT_FILL_X );

			hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
			new FXButton( hf, "Fit to Surface", nullptr, nullptr, BUTTON_NORMAL | LAYOUT_FILL_X );
			new FXButton( hf, "Reset", nullptr, nullptr, BUTTON_NORMAL | LAYOUT_FILL_X );
		}

		~SurfaceInspector() override = default;

		void set_current( ApeBrushFace *face )
		{
			this->face = face;
			if ( this->face == nullptr )
			{
				return;
			}

			if ( preview != nullptr )
			{
				delete preview;
				preview = nullptr;
			}

#if 0
			const ApeMaterial *material     = face->material;
			const char        *materialPath = ape_material_get_path( material );
			preview                         = ape_material_load_preview( materialPath );
			if ( preview != nullptr )
			{
				PLImage *smallImage = PlResizeImage( preview, 64, 64 );
				if ( smallImage != nullptr )
				{
					PlDestroyImage( preview );
					preview = smallImage;

					FXIcon *icon = new FXIcon( getApp(), reinterpret_cast< FXColor * >( preview->data[ 0 ] ), 0, IMAGE_KEEP | IMAGE_ALPHACOLOR,
					                           static_cast< int >( preview->width ),
					                           static_cast< int >( preview->height ) );
					icon->create();

					previewIcon->setIcon( icon );
				}
			}
#endif

			scaleFieldX->setText( std::to_string( this->face->materialScale.x ).c_str() );
			scaleFieldY->setText( std::to_string( this->face->materialScale.y ).c_str() );

			offsetFieldX->setText( std::to_string( this->face->materialOffset.x ).c_str() );
			offsetFieldY->setText( std::to_string( this->face->materialOffset.y ).c_str() );

			rotationField->setText( std::to_string( this->face->materialAngle.x ).c_str() );

			recalc();
		}

		long on_update( FXObject *, FXSelector, void * )
		{
			if ( face == nullptr )
			{
				return TRUE;
			}

			PLVector2 scale;
			scale.x = std::strtof( scaleFieldX->getText().text(), nullptr );
			scale.y = std::strtof( scaleFieldY->getText().text(), nullptr );

			PLVector2 offset;
			offset.x = std::strtof( offsetFieldX->getText().text(), nullptr );
			offset.y = std::strtof( offsetFieldY->getText().text(), nullptr );

			PLVector3 rotation = {};
			rotation.x         = std::strtof( rotationField->getText().text(), nullptr );

			ape_brush_face_apply_material_coordinates( face, &scale, &offset, &rotation );

			return TRUE;
		}
	};

	FXDEFMAP( SurfaceInspector )
	surfaceInspectorMap[] = {
	        FXMAPFUNC( SEL_CHANGED, SurfaceInspector::ID_SURFACE_PROPERTY, SurfaceInspector::on_update ),
	};

	FXIMPLEMENT( SurfaceInspector, FXDialogBox, surfaceInspectorMap, ARRAYNUMBER( surfaceInspectorMap ) )
}// namespace forge

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

FXDEFMAP( forge::WorldEditor )
worldEditorMap[] = {
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_POLY_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_FACE_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_EDGE_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_VERTEX_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_TRANSFORM_MODE, forge::WorldEditor::on_change_geometry_mode ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_SAVE, forge::WorldEditor::on_room_save ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_SELECT, forge::WorldEditor::on_room_select ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_NEW, forge::WorldEditor::on_new_room ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_ADD, forge::WorldEditor::on_add_room ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_EDIT, forge::WorldEditor::on_edit_room ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_DELETE, forge::WorldEditor::on_remove_room ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_GRID_UP, forge::WorldEditor::on_shift_grid ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_GRID_DOWN, forge::WorldEditor::on_shift_grid ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_GRID_ALIGN, forge::WorldEditor::on_shift_grid ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_MATERIAL_BROWSER, forge::WorldEditor::on_material_browser ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_OPEN_PROPERTIES, forge::WorldEditor::on_properties ),
};
FXIMPLEMENT( forge::WorldEditor, EditorTab, worldEditorMap, ARRAYNUMBER( worldEditorMap ) )

forge::WorldEditor::WorldEditor( FXTabBook *owner, const FXString &worldName, ApeWorld *world )
    : EditorTab( owner, "Room Editor", forge_cachedIcons[ FORGE_ICON_TYPE_ROOM ], APE_EDITOR_MODE_WORLD ),
      _gridSizeTarget( this->instance.grid.size ),
      _gridHideTarget( this->instance.grid.visible )
{
	auto *middleFrame = new FXVerticalFrame( owner, FRAME_RAISED | LAYOUT_FILL );

#if 0
	auto *menuBar  = new FXMenuBar( middleFrame, LAYOUT_SIDE_TOP | LAYOUT_FILL_X );
	auto *menuPane = new FXMenuPane( menuBar->getParent() );
	new FXMenuCommand( menuPane, "Import Room...\t\tImport an existing room.", forge::load_fx_icon( getApp(), "resources/open_model.gif" ), this, 0 );
	new FXMenuCommand( menuPane, "Import Brush...\t\tImport an existing brush.", forge::load_fx_icon( getApp(), "resources/open_model.gif" ), this, 0 );
	new FXMenuSeparator( menuPane );
	new FXMenuTitle( menuBar, "&File", nullptr, menuPane );
#endif

	auto *toolbar = new FXToolBar( middleFrame, FRAME_RAISED | FRAME_THICK );

	// room selection box
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/new_room.gif" ), this, ID_ROOM_NEW );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/save.gif" ), this, ID_ROOM_SAVE );
	roomSelectBox = new FXComboBox( toolbar, 16, this, ID_ROOM_SELECT, COMBOBOX_STATIC | FRAME_SUNKEN | FRAME_THICK | LAYOUT_CENTER_Y | LAYOUT_FILL_COLUMN | LAYOUT_MIN_WIDTH, 0, 0, 400 );
	roomSelectBox->setNumVisible( 8 );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/room_edit.gif" ), this, ID_ROOM_EDIT );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/room_add.gif" ), this, ID_ROOM_ADD );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/room_remove.gif" ), this, ID_ROOM_DELETE );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/material.gif" ), this, ID_MATERIAL_BROWSER );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/entity_edit.gif" ), this, ID_OPEN_PROPERTIES );

	new FXVerticalSeparator( toolbar );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_PLOT ]      = new FXToggleButton( toolbar, "", "", load_fx_icon( getApp(), "resources/edit_polygon.gif" ), nullptr, this, ID_POLY_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_VERTEX ]    = new FXToggleButton( toolbar, "", "", load_fx_icon( getApp(), "resources/edit_vertex.gif" ), nullptr, this, ID_VERTEX_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_FACE ]      = new FXToggleButton( toolbar, "", "", load_fx_icon( getApp(), "resources/face_mode.gif" ), nullptr, this, ID_FACE_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_TRANSFORM ] = new FXToggleButton( toolbar, "", "", load_fx_icon( getApp(), "resources/transform.gif" ), nullptr, this, ID_TRANSFORM_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ this->instance.geometryMode ]->setState( true );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/group.gif" ) );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/ungroup.gif" ) );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", load_fx_icon( getApp(), "resources/grid.gif" ), nullptr, &_gridHideTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/grid_up.gif" ), this, ID_GRID_UP );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/grid_down.gif" ), this, ID_GRID_DOWN );
	//new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/grid_orient.gif" ), this, ID_GRID_ALIGN );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_READONLY | TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", load_fx_icon( getApp(), "resources/play.gif" ) );

	auto *hs = new FX4Splitter( middleFrame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );

#if 1
	for ( unsigned int i = 0; i < APE_EDITOR_MAX_VIEWPORTS; ++i )
	{
		viewports[ i ] = new WorldViewport( hs, get_shared_gl_visual(), this, static_cast< ApeCameraViewMode >( APE_CAMERA_MODE_PERSPECTIVE + i ) );
	}
#else
	new WorldViewport( hs, get_shared_gl_visual(), this, APE_CAMERA_MODE_PERSPECTIVE );
#endif

	owner->create();

	this->_world = world;

	if ( !worldName.empty() )
	{
		setText( "Room Editor (" + worldName + ")" );
	}

	ape_editor_set_active_instance( &this->instance );
}

forge::WorldEditor::~WorldEditor()
{
	delete nodeTree;
	delete roomSelectBox;
	delete surfaceInspector;

	for ( auto &viewport : viewports )
	{
		delete viewport;
	}
}

void forge::WorldEditor::create_new_object( const char *name, ApeWorldNodeType type )
{
	FXTreeItem *selectedItem = nodeTree->getCurrentItem();
	if ( selectedItem == nullptr )
	{
		return;
	}

	auto *parentNode = ( ApeWorldNode * ) nodeTree->getCurrentItem()->getData();
	if ( parentNode == nullptr )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create object, no valid node selected!" );
		return;
	}

#if 0
	static const PLColourF32 colour = ( PLColourF32 ){ 1.0f, 1.0f, 1.0f, 1.0f };

	void *data = nullptr;
	switch ( type )
	{
		default:
			break;
		case APE_WORLD_NODE_TYPE_EMPTY:
			break;
		case APE_WORLD_NODE_TYPE_BRUSH:
			data = ape_brush_create( parentNode, nullptr, &pos, &pl_vecOrigin3 );
			break;
		case APE_WORLD_NODE_TYPE_LIGHT:
			data = ape_create_light( parentNode, &pos, &colour, 1.0f, APE_LIGHT_TYPE_OMNI, APE_LIGHT_FLAG_ENABLED );
			break;
		case APE_WORLD_NODE_TYPE_CAMERA:
			data = ape_create_camera( parentNode, nullptr, &pos, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
			break;
		case APE_WORLD_NODE_TYPE_ENTITY: break;
	}
	assert( data != nullptr );
#endif

	update_tree();
}

void forge::WorldEditor::update_tree()
{
	ApeWorldNode *child;
	PL_ITERATE_LINKED_LIST( child, ApeWorldNode, _world->base.children, i )
	{
		if ( child->type != APE_WORLD_NODE_TYPE_ROOM )
		{
			continue;
		}

		if ( roomSelectBox->findItemByData( child ) != -1 )
		{
			continue;
		}

		const char *name = ape_world_node_get_name( child );
		roomSelectBox->appendItem( name, child );
	}
}

long forge::WorldEditor::on_change_geometry_mode( FXObject *, FXSelector selector, void * )
{
	ApeEditorGeometryMode geometryMode;
	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_POLY_MODE:
			geometryMode = APE_EDITOR_GEOMETRY_MODE_PLOT;
			break;
		case ID_FACE_MODE:
			geometryMode = APE_EDITOR_GEOMETRY_MODE_FACE;
			break;
		//case ID_EDGE_MODE:
		//	this->instance.geometryMode = APE_EDITOR_GEOMETRY_MODE_EDGE;
		//	break;
		case ID_VERTEX_MODE:
			geometryMode = APE_EDITOR_GEOMETRY_MODE_VERTEX;
			break;
		case ID_TRANSFORM_MODE:
			geometryMode = APE_EDITOR_GEOMETRY_MODE_TRANSFORM;
			break;
	}

	for ( unsigned int i = 0; i < APE_EDITOR_MAX_GEOMETRY_MODES; ++i )
	{
		geometryModeButtons[ i ]->setState( geometryMode == i );
	}

	ape_editor_set_geometry_mode( &this->instance, geometryMode );

	return TRUE;
}

long forge::WorldEditor::on_shift_grid( FXObject *, FXSelector selector, void * )
{
	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_GRID_UP:
		{
			ape_grid_move_forward( &instance.grid );
			break;
		}
		case ID_GRID_DOWN:
		{
			ape_grid_move_backward( &instance.grid );
			break;
		}
	}
	return TRUE;
}

long forge::WorldEditor::on_room_save( FXObject *, FXSelector, void * )
{
	ApeRoom *room = get_active_room();
	if ( room == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "No active room currently selected!" );
		return false;
	}

	if ( savePath.empty() )
	{
		PLPath origin;
		PlSetupPath( origin, true, "%s/dev/rooms/<room>", com_project_get_local_path() );

		FXString saveFilename = FXFileDialog::getSaveFilename( this, "Save Room", origin, "*." APE_WORLD_ROOM_EXTENSION );
		if ( saveFilename.empty() )
		{
			return false;
		}

		savePath = saveFilename.text();

		// add the extension if it's missing
		if ( savePath.substr( savePath.size() - strlen( "." APE_WORLD_ROOM_EXTENSION ) ) != "." APE_WORLD_ROOM_EXTENSION )
		{
			savePath += ".rom" ACM_DEFAULT_EXTENSION_OLD;
		}
	}

	AcmBranch *root = ape_world_node_serialize( APE_WORLD_NODE( room ), nullptr );
	if ( root == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to serialize room!" );
		return false;
	}

	const char *path = savePath.c_str();
	if ( !acm_write_file( path, root, ACM_FILE_TYPE_BINARY ) )
	{
		FXMessageBox::warning( this, MBOX_OK, "Warning", "%s", acm_get_error_message() );
		return false;
	}

	return true;
}

long forge::WorldEditor::on_room_select( FXObject *, FXSelector, void * )
{
	const FXint current = roomSelectBox->getCurrentItem();
	ApeRoom    *room    = static_cast< ApeRoom * >( roomSelectBox->getItemData( current ) );
	if ( room == nullptr )
	{
		return FALSE;
	}

	set_active_room( room );

	return TRUE;
}

long forge::WorldEditor::on_new_room( FXObject *, FXSelector, void * )
{
	//TODO
	return FALSE;
}

long forge::WorldEditor::on_add_room( FXObject *, FXSelector, void * )
{
	const char *path     = com_project_get_local_path();
	FXString    filename = FXFileDialog::getOpenFilename( this, "Select a room", FXString( path ) + "/dev/rooms/", "*." APE_WORLD_ROOM_EXTENSION );
	if ( filename.empty() )
	{
		return FALSE;
	}

	AcmBranch *root = acm_load_file( filename.text(), "node" );
	if ( root == nullptr )
	{
		return FALSE;
	}

	ApeWorldNode *roomNode = ape_world_node_deserialize( APE_WORLD_NODE( _world ), root );
	if ( roomNode == nullptr )
	{
		return FALSE;
	}

	if ( roomNode->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Selected file is not a valid room file!" );
		ape_world_node_destroy( APE_WORLD_NODE( roomNode ) );
		return FALSE;
	}

	update_tree();

	return TRUE;
}

long forge::WorldEditor::on_edit_room( FXObject *, FXSelector, void * )
{
	assert( activeRoom != nullptr );

	RoomDialog roomCreationDialog( this, activeRoom );
	if ( roomCreationDialog.execute() )
	{
		const FXString roomName = roomCreationDialog.get_room_name();
		if ( roomName.empty() )
		{
			FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "No name specified for room!" );
			return false;
		}

		ape_world_node_set_name( APE_WORLD_NODE( activeRoom ), roomName.text() );
		ape_room_set_ambience( activeRoom, roomCreationDialog.get_room_ambience() );
		ape_room_set_reverb_preset( activeRoom, roomCreationDialog.get_room_audio_preset() );

		update_tree();

		return true;
	}

	return false;
}

long forge::WorldEditor::on_remove_room( FXObject *, FXSelector, void * )
{
}

void forge::WorldEditor::set_active_room( ApeRoom *room )
{
	if ( activeRoom == room )
	{
		return;
	}

	for ( auto *viewport : viewports )
	{
		ape_camera_set_room( viewport->camera, room );
	}

	activeRoom = room;
}

long forge::WorldEditor::on_material_browser( FX::FXObject *, FX::FXSelector, void * )
{
	mainWindow->open_material_browser();
	return TRUE;
}

long forge::WorldEditor::on_properties( FXObject *, FXSelector, void * )
{
	ApeWorldNode      *node;
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance != nullptr && instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
	{
		node = static_cast< ApeWorldNode * >( ape_editor_get_first_selected( instance ) );
	}
	else
	{
		node = nullptr;
	}

	mainWindow->open_properties( node );

	return TRUE;
}

void forge::WorldEditor::open_face_inspector()
{
	if ( instance.geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return;
	}

	ApeEditorInstance *instance = ape_editor_get_active_instance();
	assert( instance != nullptr );

	ApeBrushFace *face = static_cast< ApeBrushFace * >( ape_editor_get_first_selected( instance ) );
	if ( face == nullptr )
	{
		return;
	}

	if ( surfaceInspector == nullptr )
	{
		surfaceInspector = new SurfaceInspector( this );
		surfaceInspector->create();
	}

	surfaceInspector->set_current( face );
	surfaceInspector->show();
}

void forge::WorldEditor::set_face_inspector_surface( ApeBrushFace *face )
{
	if ( surfaceInspector == nullptr )
	{
		return;
	}

	surfaceInspector->set_current( face );

	if ( face == nullptr )
	{
		surfaceInspector->hide();
	}
}

ApeRoom *forge::WorldEditor::get_active_room()
{
	FXint current = roomSelectBox->getCurrentItem();
	if ( current == -1 )
	{
		return nullptr;
	}

	ApeRoom *room = static_cast< ApeRoom * >( roomSelectBox->getItemData( current ) );
	if ( room == nullptr )
	{
		return nullptr;
	}

	return room;
}

void forge::WorldEditor::autosave()
{
	ApeRoom *room = get_active_room();
	if ( room == nullptr )
	{
		forge_warning_( "Skipping autosave, no active room!\n" );
		return;
	}

	PLPath path;
	PlSetupPath( path, true, "%s/dev/rooms/autosave." APE_WORLD_ROOM_EXTENSION, com_project_get_local_path() );

	AcmBranch *root = ape_world_node_serialize( APE_WORLD_NODE( room ), nullptr );
	if ( root == nullptr )
	{
		forge_warning_( "Failed to serialize room!\n" );
		return;
	}

	if ( !acm_write_file( path, root, ACM_FILE_TYPE_BINARY ) )
	{
		FXMessageBox::warning( this, MBOX_OK, "Warning", "%s", acm_get_error_message() );
	}

	forge_print_( "Autosave completed: %s\n", path );
}

/////////////////////////////////////////////////////////////////////////////////////
// Room Creation Dialog

FXDEFMAP( forge::WorldEditor::RoomDialog )
roomCreationMap[] = {};
FXIMPLEMENT( forge::WorldEditor::RoomDialog, FXDialogBox, roomCreationMap, ARRAYNUMBER( roomCreationMap ) )

forge::WorldEditor::RoomDialog::RoomDialog( FXWindow *parent, ApeRoom *room ) : FXDialogBox( parent, "Room Properties", DECOR_TITLE | DECOR_BORDER )
{
	FXMatrix *matrix = new FXMatrix( this, 2, MATRIX_BY_COLUMNS );

	new FXLabel( matrix, "Name:" );
	nameField = new FXTextField( matrix, 20 );

	new FXLabel( matrix, "Audio Preset:" );
	audioPresetField = new FXListBox( matrix );
	audioPresetField->setNumVisible( 8 );
	for ( unsigned int i = 0; i < APE_NUM_AUDIO_EFFECT_TYPES; ++i )
	{
		audioPresetField->appendItem( APE_AUDIO_EFFECT_TYPES[ i ].name );
	}

	new FXLabel( matrix, "Ambience:" );
	ambienceField = new FXColorWell( matrix, FXRGB( 0, 0, 0 ) );

	new FXSeparator( this );

	FXHorizontalFrame *buttonFrame = new FXHorizontalFrame( this, LAYOUT_SIDE_RIGHT | PACK_UNIFORM_WIDTH );
	new FXButton( buttonFrame, ( room == nullptr ) ? "Create" : "&OK", nullptr, this, FXDialogBox::ID_ACCEPT, BUTTON_NORMAL | BUTTON_INITIAL );
	new FXButton( buttonFrame, "&Cancel", nullptr, this, ID_CANCEL, BUTTON_NORMAL );

	// if we've got a room, populate everything
	if ( room != nullptr )
	{
		const char *name = ape_world_node_get_name( APE_WORLD_NODE( room ) );
		assert( name != nullptr );
		nameField->setText( name );

		ApeAudioReverbPreset reverbPreset = ape_room_get_reverb_preset( room );
		audioPresetField->setCurrentItem( reverbPreset );

		PLColourF32 ambience = ape_room_get_ambience( room );
		ambienceField->setRGBA( FXRGBA(
		        // sigh...
		        PlFloatToByte( ambience.r ),
		        PlFloatToByte( ambience.g ),
		        PlFloatToByte( ambience.b ),
		        PlFloatToByte( ambience.a ) ) );
	}
}
