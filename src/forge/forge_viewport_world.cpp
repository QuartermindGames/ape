// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Forge world viewport implementation.
// Author:  Mark E. Sowden

#include "forge.h"
#include "forge_viewport_world.h"
#include "forge_editor_world.h"
#include "forge_window_main.h"

#include "common_project.h"

#include "ape/ape_public_model.h"
#include "ape/ape_formats.h"
#include "yin/core_entity.h"

/////////////////////////////////////////////////////////////////////////////////////
// Entity Selection Dialog
/////////////////////////////////////////////////////////////////////////////////////

namespace forge
{
	class EntitySelectionDialog final : public FXDialogBox
	{
		FXDECLARE( EntitySelectionDialog )

		FXListBox *list;

	protected:
		EntitySelectionDialog() = default;

	public:
		explicit EntitySelectionDialog( FXWindow *parent ) : FXDialogBox( parent, "Select Entity Class" )
		{
			setWidth( 256 );

			FXVerticalFrame *frame = new FXVerticalFrame( this, LAYOUT_FILL );

			unsigned int                     numClasses;
			const ApeEntityClassDefinition **classes = ape_entity_get_classes( &numClasses );
			assert( numClasses != 0 && classes != nullptr );

			list = new FXListBox( frame, nullptr, 0, FRAME_SUNKEN | FRAME_THICK | LAYOUT_FILL_X | LISTBOX_NORMAL );
			for ( unsigned int i = 0; i < numClasses; ++i )
			{
				assert( classes[ i ] != nullptr );
				if ( classes[ i ]->excludeInEditor )
				{
					continue;
				}

				list->appendItem( classes[ i ]->name, icon, ( void * ) classes[ i ] );
			}
			list->setNumVisible( PlClamp( 4, list->getNumItems(), 8 ) );

			new FXHorizontalSeparator( frame );

			auto *hf = new FXHorizontalFrame( frame, LAYOUT_FILL | LAYOUT_RIGHT );
			new FXButton( hf, "Accept", nullptr, this, ID_ACCEPT, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
			new FXButton( hf, "Cancel", nullptr, this, ID_CANCEL, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
		}

		~EntitySelectionDialog() override = default;

		[[nodiscard]] const ApeEntityClassDefinition *get_selected() const
		{
			const int item = list->getCurrentItem();
			if ( item == -1 )
			{
				return nullptr;
			}

			const ApeEntityClassDefinition *cls = static_cast< const ApeEntityClassDefinition * >( list->getItemData( item ) );
			assert( cls != nullptr );
			return cls;
		}
	};

	FXIMPLEMENT( EntitySelectionDialog, FXDialogBox, nullptr, 0 )
}// namespace forge

/////////////////////////////////////////////////////////////////////////////////////

FXDEFMAP( forge::WorldViewport )
worldViewportMap[] = {
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_GRID_ALIGN, forge::WorldViewport::on_grid_align ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_INSPECTOR, forge::WorldViewport::on_face_inspector ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_TOGGLE, forge::WorldViewport::on_face_toggle ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_TOGGLE_OTHERS, forge::WorldViewport::on_face_toggle ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_SHADE_SMOOTH, forge::WorldViewport::on_face_shade_smooth ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_SHADE_FLAT, forge::WorldViewport::on_face_shade_flat ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_FLIP, forge::WorldViewport::on_face_flip ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_LINK_NEW_ROOM, forge::WorldViewport::on_link_new_room ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_UNLINK_PORTAL, forge::WorldViewport::on_face_unlink_portal ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_LINK_PORTAL, forge::WorldViewport::on_face_link_portal ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_FLAG_MIRROR, forge::WorldViewport::on_toggle_face_flag ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_MOVE_NODE_TO_ROOM, forge::WorldViewport::on_move_node_to_room ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_MODEL, forge::WorldViewport::on_create_node ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_LIGHT, forge::WorldViewport::on_create_node ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_CAMERA, forge::WorldViewport::on_create_node ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_ENTITY, forge::WorldViewport::on_create_node ),
};
FXIMPLEMENT( forge::WorldViewport, forge::Viewport, worldViewportMap, ARRAYNUMBER( worldViewportMap ) )

forge::WorldViewport::WorldViewport( FXComposite *composite, FXGLVisual *visual, WorldEditor *editor, ApeCameraViewMode viewMode )
    : Viewport( composite, visual, editor, viewMode )
{
}

long forge::WorldViewport::on_left_click( FXObject *object, FXSelector selector, void *ptr )
{
	if ( Viewport::on_left_click( object, selector, ptr ) )
	{
		return TRUE;
	}

	if ( !hasFocus() )
	{
		return FALSE;
	}

	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );

	if ( FXSELTYPE( selector ) != SEL_LEFTBUTTONPRESS )
	{
		return FALSE;
	}

	auto *event = static_cast< FXEvent * >( ptr );
	if ( !( event->state & CONTROLMASK ) )
	{
		ape_editor_clear_selection( instance );
	}

	switch ( instance->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_PLOT:
		{
			ape_editor_add_polygon_point( instance );
			return TRUE;
		}
		case APE_EDITOR_GEOMETRY_MODE_VERTEX:
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			WorldEditor *worldEditor = dynamic_cast< WorldEditor * >( editor );
			if ( worldEditor == nullptr )
			{
				return false;
			}

			void *p = ape_editor_get_object_under_cursor( instance );
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
			{
				worldEditor->set_face_inspector_surface( ( ApeBrushFace * ) p );
			}

			ape_editor_add_object_to_selection( instance, p );
			return true;
		}
	}

	return FALSE;
}

long forge::WorldViewport::on_right_click( FXObject *object, FXSelector selector, void *ptr )
{
	if ( Viewport::on_right_click( object, selector, ptr ) )
	{
		return TRUE;
	}

	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );

	auto event = static_cast< FXEvent * >( ptr );
	if ( event->moved )
	{
		return TRUE;
	}

	FXMenuPane *popup = nullptr;
	switch ( instance->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_PLOT:
		{
			if ( instance->numPolygonPoints > 0 )
			{
				ape_editor_brush_from_polygon( instance, mainWindow->get_active_material() );
				return TRUE;
			}

			PLVector2 pos;
			if ( ape_grid_get_cursor_position( &instance->grid, &pos ) == nullptr )
			{
				return TRUE;
			}

			popup = new FXMenuPane( this );

			unsigned int              numClasses;
			const ApeWorldNodeClass **classes = ape_world_node_get_classes( &numClasses );
			assert( numClasses != 0 && classes != nullptr );
			for ( unsigned int i = 0; i < numClasses; ++i )
			{
				if ( classes[ i ] == nullptr || classes[ i ]->flags & APE_WORLD_NODE_CLASS_FLAG_NO_EDITOR )
				{
					continue;
				}

				FXIcon *icon;
				if ( classes[ i ]->editorIcon != nullptr )
				{
					icon = load_fx_icon( getApp(), classes[ i ]->editorIcon );
				}
				else
				{
					icon = nullptr;
				}

				FXString const &identifier = classes[ i ]->identifier;
				new FXMenuCommand( popup, FXString( "Create " ) + identifier, icon, this, ID_CREATE_NODE + i );
			}

			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_VERTEX: break;
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			ApeBrushFace *face = ( ApeBrushFace * ) ape_editor_get_first_selected( instance );
			if ( face == nullptr )
			{
				return true;
			}

			// Create a pop-up menu
			popup = new FXMenuPane( this );
			new FXMenuCommand( popup, "Inspector", load_fx_icon( getApp(), "resources/silk/zoom.png" ), this, ID_FACE_INSPECTOR );
			new FXMenuSeparator( popup );
			new FXMenuCommand( popup, "Toggle Faces", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_TOGGLE ], this, ID_FACE_TOGGLE );
			new FXMenuCommand( popup, "Toggle Other Faces", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_TOGGLE_OTHER ], this, ID_FACE_TOGGLE_OTHERS );
			new FXMenuSeparator( popup );
			new FXMenuCommand( popup, "Flip Faces", forge_cachedIcons[ FORGE_ICON_TYPE_MODE_FACE ], this, ID_FACE_FLIP );
			new FXMenuSeparator( popup );
			new FXMenuCommand( popup, "Shade Faces Smooth", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_SMOOTH ], this, ID_FACE_SHADE_SMOOTH );
			new FXMenuCommand( popup, "Shade Faces Flat", load_fx_icon( getApp(), "resources/face_flat.gif" ), this, ID_FACE_SHADE_FLAT );
			new FXMenuSeparator( popup );
			new FXMenuCommand( popup, "Align Grid to Face", forge_cachedIcons[ FORGE_ICON_TYPE_GRID_ORIENT ], this, ID_GRID_ALIGN );

			new FXMenuSeparator( popup );

			ApeRoom *room = ape_brush_face_get_room( face );
			if ( room != nullptr )
			{
				ApeWorldNode *rootNode = ape_world_node_get_root( APE_WORLD_NODE( room ) );
				if ( rootNode != nullptr && rootNode->type == APE_WORLD_NODE_TYPE_ROOT )
				{
					unsigned int   numTaggedSurfaces;
					ApeBrushFace **taggedSurfaces = ape_world_get_tagged_surfaces( ( ApeWorld * ) rootNode, &numTaggedSurfaces );

					FXMenuPane *linkMenu = new FXMenuPane( this );
					new FXMenuCascade( popup, "Link Portal", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_PORTAL ], linkMenu );
					new FXMenuCommand( linkMenu, "New Room...", forge_cachedIcons[ FORGE_ICON_TYPE_NEW_ROOM ], this, ID_FACE_LINK_NEW_ROOM );

					if ( numTaggedSurfaces > 0 )
					{
						new FXMenuSeparator( linkMenu );
						for ( unsigned int i = 0; i < numTaggedSurfaces; ++i )
						{
							room = ape_brush_face_get_room( taggedSurfaces[ i ] );
							assert( room != nullptr );

							const char *path = ape_room_get_path( room );
							if ( path == nullptr || *path == '\0' )
							{
								forge_warning_( "Encountered a room with an invalid path!\n" );
								continue;
							}

							new FXMenuCommand( linkMenu, FXString( path ) + ":" + taggedSurfaces[ i ]->tag, nullptr, this, ID_FACE_LINK_PORTAL );
						}
					}

					PL_DELETE( taggedSurfaces );
				}
			}

			FXMenuCommand *command = new FXMenuCommand( popup, "Unlink Portal", nullptr, this, ID_FACE_UNLINK_PORTAL );
			if ( ape_brush_face_is_mirror( face ) || !ape_brush_face_is_portal( face ) )
			{
				command->disable();
			}

			( new FXMenuCheck( popup, "Mirror", this, ID_FACE_FLAG_MIRROR ) )->setCheck( face->flags & APE_BRUSH_FACE_FLAG_MIRROR );

			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
		{
			unsigned int numSelectedNodes = PlGetNumLinkedListNodes( instance->selectedObjects );
			if ( numSelectedNodes == 0 )
			{
				return true;
			}

			popup = new FXMenuPane( this );

			FXMenuPane    *subMenu    = new FXMenuPane( this );
			FXMenuCascade *moveToMenu = new FXMenuCascade( popup, "Move to Room...", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_PORTAL ], subMenu );

			WorldEditor *worldEditor = ( WorldEditor * ) editor;

			std::vector< ApeRoom * > rooms;
			worldEditor->get_rooms( &rooms );
			if ( rooms.size() > 1 )
			{
				ApeRoom *activeRoom = worldEditor->get_active_room();
				for ( auto &i : rooms )
				{
					if ( i == activeRoom )
					{
						continue;
					}

					const char *path = ape_room_get_path( i );
					if ( path == nullptr || *path == '\0' )
					{
						// this shouldn't happen...
						forge_warning_( "Encountered a room with an invalid path!\n" );
						continue;
					}

					new FXMenuCommand( subMenu, path, nullptr, this, ID_MOVE_NODE_TO_ROOM );
				}
			}
			else
			{
				moveToMenu->disable();
			}
			break;
		}
	}

	if ( popup != nullptr )
	{
		popup->create();
		popup->popup( nullptr, event->root_x, event->root_y );
		FXApp::instance()->runModalWhileShown( popup );

		delete popup;

		return true;
	}

	return FALSE;
}

long forge::WorldViewport::on_middle_click( FXObject *fx_object, FXSelector fx_selector, void *p )
{
	if ( Viewport::on_middle_click( fx_object, fx_selector, p ) )
	{
		return TRUE;
	}

	auto event = static_cast< FXEvent * >( p );
	if ( event->moved )
	{
		return TRUE;
	}

	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );
	if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		const ApeBrushFace *face = static_cast< ApeBrushFace * >( ape_editor_get_first_selected( instance ) );
		if ( face == nullptr )
		{
			return TRUE;
		}

		ApeBrushFace *highlightedFace = static_cast< ApeBrushFace * >( ape_editor_get_object_under_cursor( instance ) );
		if ( highlightedFace == nullptr )
		{
			return TRUE;
		}

		ape_brush_face_apply_material( highlightedFace, face->material );

		const PLVector2 offset = PL_VECTOR2( face->materialOffset.x, face->materialOffset.y );
		ape_brush_face_apply_material_coordinates( highlightedFace, &face->materialScale, &offset, &face->materialAngle );
	}

	return FALSE;
}

long forge::WorldViewport::on_key( FXObject *object, FXSelector selector, void *ptr )
{
	if ( Viewport::on_key( object, selector, ptr ) )
	{
		return TRUE;
	}

	ApeEditorInstance *instance = editor->get_internal();
	if ( instance == nullptr )
	{
		return FALSE;
	}

	if ( FXSELTYPE( selector ) != SEL_KEYPRESS )
	{
		return FALSE;
	}

	auto *event = static_cast< FXEvent * >( ptr );
	switch ( event->code )
	{
		default:
			break;

		case KEY_Delete:
		{
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_PLOT )
			{
				ape_editor_remove_polygon_point( instance );
				return TRUE;
			}
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
			{
				ape_editor_delete_selection( instance );
				return TRUE;
			}
			break;
		}
		case KEY_Escape:
		{
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_PLOT )
			{
				ape_editor_clear_plot_points( instance );
				return TRUE;
			}

			ape_editor_clear_selection( instance );
			return TRUE;
		}

		// grid controls
		case KEY_bracketleft:
		case KEY_KP_Subtract:
		{
			if ( event->state & SHIFTMASK )
			{
				ape_grid_move_backward( &instance->grid );
				return TRUE;
			}
			ape_grid_decrease_size();
			return TRUE;
		}
		case KEY_bracketright:
		case KEY_KP_Add:
		{
			if ( event->state & SHIFTMASK )
			{
				ape_grid_move_forward( &instance->grid );
				return TRUE;
			}
			ape_grid_increase_size();
			return TRUE;
		}

		case FX::KEY_F1:
		{
			static_cast< WorldEditor * >( editor )->on_change_geometry_mode( nullptr, WorldEditor::ID_POLY_MODE, nullptr );
			return TRUE;
		}
		case FX::KEY_F2:
		{
			static_cast< WorldEditor * >( editor )->on_change_geometry_mode( nullptr, WorldEditor::ID_VERTEX_MODE, nullptr );
			return TRUE;
		}
		case FX::KEY_F3:
		{
			static_cast< WorldEditor * >( editor )->on_change_geometry_mode( nullptr, WorldEditor::ID_FACE_MODE, nullptr );
			return TRUE;
		}
		case FX::KEY_F4:
		{
			static_cast< WorldEditor * >( editor )->on_change_geometry_mode( nullptr, WorldEditor::ID_TRANSFORM_MODE, nullptr );
			return TRUE;
		}

		case KEY_r:
		{
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
			{
				ApeBrushFace *face = static_cast< ApeBrushFace * >( ape_editor_get_first_selected( instance ) );
				if ( face == nullptr )
				{
					return TRUE;
				}

				ape_grid_align_to_face( &instance->grid, face );
				return TRUE;
			}

			return FALSE;
		}
		case KEY_H:
		case KEY_h:
		{
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
			{
				if ( event->state & SHIFTMASK )
				{
					ape_editor_toggle_other_faces( instance );
				}
				else
				{
					ape_editor_toggle_faces( instance );
				}
				return TRUE;
			}

			return FALSE;
		}
	}

	// duplicate selection
	if ( event->state & SHIFTMASK && instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM && ( event->code == KEY_D || event->code == KEY_d ) )
	{
		ape_editor_duplicate_selection( instance );
		return true;
	}

	// allow us to use shift + arrows to move things around
	if ( event->state & SHIFTMASK && ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_VERTEX ||
	                                   instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM ||
	                                   instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE ) )
	{
		PLVector3 dir = {};
		switch ( event->code )
		{
			default:
				break;

			// shift move
			case KEY_Up:
			{
				dir.x = 1.0f;
				break;
			}
			case KEY_Down:
			{
				dir.x = -1.0f;
				break;
			}
			case KEY_Left:
			{
				dir.z = 1.0f;
				break;
			}
			case KEY_Right:
			{
				dir.z = -1.0f;
				break;
			}
		}

		if ( PlCompareVector3( &dir, &pl_vecOrigin3 ) )
		{
			return false;
		}

		ape_editor_shift_selection( instance, &dir );
	}

	return FALSE;
}

long forge::WorldViewport::on_motion( FXObject *object, FXSelector selector, void *ptr )
{
	Viewport::on_motion( object, selector, ptr );

	if ( internalViewport_ == nullptr )
	{
		return false;
	}

	ApeEditorInstance *instance = editor->get_internal();
	if ( instance == nullptr )
	{
		return false;
	}

	auto *event = ( FXEvent * ) ptr;
	ape_editor_on_mouse_move( instance, internalViewport_, event->win_x, event->win_y );

	return TRUE;
}

long forge::WorldViewport::on_grid_align( FXObject *, FXSelector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );

	if ( instance->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return FALSE;
	}

	ApeBrushFace *face = static_cast< ApeBrushFace * >( ape_editor_get_first_selected( instance ) );
	if ( face == nullptr )
	{
		return FALSE;
	}

	ape_grid_align_to_face( &instance->grid, face );
	return TRUE;
}

long forge::WorldViewport::on_face_inspector( FXObject *, FXSelector, void * )
{
	static_cast< WorldEditor * >( editor )->open_face_inspector();
	return TRUE;
}

long forge::WorldViewport::on_face_toggle( FXObject *, FXSelector selector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );

	if ( FXSELID( selector ) == ID_FACE_TOGGLE_OTHERS )
	{
		ape_editor_toggle_other_faces( instance );
	}
	else
	{
		ape_editor_toggle_faces( instance );
	}

	return TRUE;
}

long forge::WorldViewport::on_face_shade_smooth( FXObject *, FXSelector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );
	ape_editor_shade_faces_smooth( instance );
	return TRUE;
}

long forge::WorldViewport::on_face_shade_flat( FXObject *, FXSelector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );
	ape_editor_shade_faces_flat( instance );
	return TRUE;
}

long forge::WorldViewport::on_face_flip( FXObject *, FXSelector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );
	ape_editor_flip_faces( instance );
	return TRUE;
}

long forge::WorldViewport::on_link_new_room( FXObject *, FXSelector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );
	if ( instance->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return false;
	}

	ApeBrushFace *face = ( ApeBrushFace * ) ape_editor_get_first_selected( instance );
	if ( face == nullptr )
	{
		return false;
	}

	static_cast< WorldEditor * >( editor )->link_new_room( face );
	return true;
}

long forge::WorldViewport::on_face_unlink_portal( FXObject *, FXSelector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	if ( instance->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return false;
	}

	ApeBrushFace *face;
	COM_ITERATE_LINKED_LIST( face, instance->selectedObjects, i )
	{
		*face->destinationTag = '\0';
		face->flags &= ~APE_BRUSH_FACE_FLAG_PORTAL;
	}

	return true;
}

long forge::WorldViewport::on_face_link_portal( FXObject *object, FXSelector, void * )
{
	FXMenuCommand *command = dynamic_cast< FXMenuCommand * >( object );
	if ( command == nullptr )
	{
		return false;
	}

	std::string tag = command->getText().text();
	if ( tag.empty() )
	{
		return false;
	}

	ApeEditorInstance *instance = editor->get_internal();
	if ( instance->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return false;
	}

	ApeBrushFace *face;
	COM_ITERATE_LINKED_LIST( face, instance->selectedObjects, i )
	{
		snprintf( face->destinationTag, sizeof( face->destinationTag ), "%s", tag.c_str() );
		face->flags |= APE_BRUSH_FACE_FLAG_PORTAL;
	}

	return true;
}

long forge::WorldViewport::on_move_node_to_room( FXObject *object, FXSelector, void * )
{
	FXMenuCommand *command = dynamic_cast< FXMenuCommand * >( object );
	if ( command == nullptr )
	{
		return false;
	}

	WorldEditor *worldEditor = dynamic_cast< WorldEditor * >( editor );
	if ( worldEditor == nullptr )
	{
		return false;
	}

	ApeWorld *world = worldEditor->get_world();
	if ( world == nullptr )
	{
		return false;
	}

	std::string path = command->getText().text();
	ApeRoom    *room = ape_world_get_room_by_path( world, path.c_str() );
	if ( room == nullptr )
	{
		forge_warning_( "Failed to find room (%s)!\n", path.c_str() );
		return false;
	}

	ApeEditorInstance *instance = worldEditor->get_internal();
	if ( instance == nullptr )
	{
		forge_warning_( "Invalid editor instance!\n" );
		return false;
	}

	ape_editor_move_selected_to_room( instance, room );

	return true;
}

long forge::WorldViewport::on_toggle_face_flag( FXObject *object, FXSelector selector, void * )
{
	FXMenuCheck *checkBox = dynamic_cast< FXMenuCheck * >( object );
	if ( checkBox == nullptr )
	{
		return FALSE;
	}

	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );

	ApeBrushFace *face = static_cast< ApeBrushFace * >( ape_editor_get_first_selected( instance ) );
	if ( face == nullptr )
	{
		return FALSE;
	}

	ApeRoom *room = ape_brush_face_get_room( face );
	assert( room != nullptr );

	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_FACE_FLAG_MIRROR:
		{
			if ( checkBox->getCheck() )
			{
				face->flags |= APE_BRUSH_FACE_FLAG_MIRROR;
			}
			else
			{
				face->flags &= ~APE_BRUSH_FACE_FLAG_MIRROR;
			}
			return TRUE;
		}
	}

	return FALSE;
}

long forge::WorldViewport::on_create_node( FXObject *, FXSelector sel, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );

	// attempt to fetch the room (todo: this api sucks...)
	ApeRoom *room = ape_camera_get_room( instance->camera );
	assert( room != nullptr );

	// check for the current grid cursor position
	PLVector2 pos;
	if ( ape_grid_get_cursor_position( &instance->grid, &pos ) == nullptr )
	{
		return TRUE;
	}

	// now get the transformed position
	PLVector3 tpos = ape_grid_transform_point( &instance->grid, &pos );
	switch ( FXSELID( sel ) )
	{
		default:
		{
			printf( "Unknown selection ID (%u)\n", FXSELID( sel ) );
			break;
		}
		case ID_CREATE_NODE + APE_WORLD_NODE_TYPE_MODEL:
		{
			const char *path     = com_project_get_local_path();
			FXString    filename = FXFileDialog::getOpenFilename( this, "Select a model", FXString( path ) + "/ship/models/", "*." APE_FORMAT_MODEL_EXTENSION );
			if ( filename.empty() )
			{
				break;
			}

			PLFileSystemMount *mount = PlGetMountLocationForPath( filename.text() );
			if ( mount == nullptr )
			{
				forge_warning_( "Model (%s) must be placed under a mounted location!\n", path );
				break;
			}

			const char   *mountPath = PlGetMountLocationPath( mount );
			ApeModelNode *node      = ape_model_node_create( APE_WORLD_NODE( room ), nullptr, &filename[ strlen( mountPath ) + 1 ] );
			if ( node == nullptr )
			{
				break;
			}

			ape_world_node_set_position( APE_WORLD_NODE( node ), &tpos );
			break;
		}
		case ID_CREATE_NODE + APE_WORLD_NODE_TYPE_LIGHT:
		{
			static constexpr PLColourF32 DEFAULT_COLOUR = PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.0f );
			ape_create_light( APE_WORLD_NODE( room ), &tpos, &DEFAULT_COLOUR, 128.0f, APE_LIGHT_TYPE_OMNI, APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_RUNTIME_SHADOWS | APE_LIGHT_FLAG_SHADOWS );
			break;
		}
		case ID_CREATE_NODE + APE_WORLD_NODE_TYPE_CAMERA:
		{
			ape_create_camera( APE_WORLD_NODE( room ), nullptr, &tpos, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
			break;
		}
		case ID_CREATE_NODE + APE_WORLD_NODE_TYPE_ENTITY:
		{
			EntitySelectionDialog dialog = EntitySelectionDialog( this );
			if ( dialog.execute() )
			{
				const ApeEntityClassDefinition *cls = dialog.get_selected();
				if ( cls == nullptr )
				{
					break;
				}

				ape_entity_create( APE_WORLD_NODE( room ), cls->name, nullptr, nullptr, &tpos, &pl_vecOrigin3 );
			}
			break;
		}
	}

	return TRUE;
}
