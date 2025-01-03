// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Forge world viewport implementation.
// Author:  Mark E. Sowden

#include "forge.h"
#include "forge_viewport_world.h"
#include "forge_editor_world.h"
#include "forge_window_main.h"

#include <yin/core_entity.h>

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

				list->appendItem( classes[ i ]->name, icon, &classes[ i ] );
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
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_TOGGLE, forge::WorldViewport::on_face_toggle ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_TOGGLE_OTHERS, forge::WorldViewport::on_face_toggle ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_FACE_FLIP, forge::WorldViewport::on_face_flip ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_MODEL, forge::WorldViewport::on_create_node ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_LIGHT, forge::WorldViewport::on_create_node ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_CAMERA, forge::WorldViewport::on_create_node ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldViewport::ID_CREATE_NODE + APE_WORLD_NODE_TYPE_ENTITY, forge::WorldViewport::on_create_node ),
};
FXIMPLEMENT( forge::WorldViewport, forge::Viewport, worldViewportMap, ARRAYNUMBER( worldViewportMap ) )

forge::WorldViewport::WorldViewport( FXComposite *composite, FXGLVisual *visual, forge::WorldEditor *editor, ApeCameraViewMode viewMode )
    : Viewport( composite, visual, editor, viewMode )
{
}

long forge::WorldViewport::on_left_click( FXObject *object, FXSelector selector, void *ptr )
{
	if ( Viewport::on_left_click( object, selector, ptr ) )
	{
		return TRUE;
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
			void *p = ape_editor_get_object_under_cursor( instance );
			if ( p == nullptr )
			{
				return TRUE;
			}

			ape_editor_add_object_to_selection( instance, p );
			return TRUE;
		}
	}

	return FALSE;
}

long forge::WorldViewport::on_right_click( FX::FXObject *object, FX::FXSelector selector, void *ptr )
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

			FXMenuPane *popup = new FXMenuPane( this );

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

			popup->create();
			popup->popup( nullptr, event->root_x, event->root_y );
			getApp()->runModalWhileShown( popup );

			delete popup;

			return TRUE;
		}
		case APE_EDITOR_GEOMETRY_MODE_VERTEX: break;
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			if ( PlGetNumLinkedListNodes( instance->selectedObjects ) == 0 )
			{
				return TRUE;
			}

			// Create a pop-up menu
			FXMenuPane *popup = new FXMenuPane( this );
			new FXMenuCommand( popup, "Toggle Faces", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_TOGGLE ], this, ID_FACE_TOGGLE );
			new FXMenuCommand( popup, "Toggle Other Faces", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_TOGGLE_OTHER ], this, ID_FACE_TOGGLE_OTHERS );
			new FXMenuSeparator( popup );
			new FXMenuCommand( popup, "Flip Faces", forge_cachedIcons[ FORGE_ICON_TYPE_MODE_FACE ], this, ID_FACE_FLIP );
			//new FXMenuCommand( popup, "Smooth Faces", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_SMOOTH ], this, ID_FACE_SMOOTH );
			new FXMenuSeparator( popup );
			new FXMenuCommand( popup, "Align Grid to Face", forge_cachedIcons[ FORGE_ICON_TYPE_GRID_ORIENT ], this, ID_GRID_ALIGN );
			//new FXMenuSeparator( popup );
			//new FXMenuCommand( popup, "Set ID...", forge_cachedIcons[ FORGE_ICON_TYPE_MODE_BRUSH ], this, ID_BUTTON_CREATE_BRUSH );
			//new FXMenuCommand( popup, "Connect Portal...", forge_cachedIcons[ FORGE_ICON_TYPE_FACE_PORTAL ], this, ID_BUTTON_CREATE_BRUSH );

			// Show the menu
			popup->create();
			popup->popup( nullptr, event->root_x, event->root_y );
			getApp()->runModalWhileShown( popup );

			delete popup;
			return TRUE;
		}
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM: break;
	}

	return FALSE;
}

long forge::WorldViewport::on_key( FX::FXObject *object, FX::FXSelector selector, void *ptr )
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
			else if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
			{
				ape_editor_delete_selection( instance );
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
			break;
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

	return FALSE;
}

long forge::WorldViewport::on_motion( FXObject *object, FXSelector selector, void *ptr )
{
	Viewport::on_motion( object, selector, ptr );

	auto     *event = ( FXEvent * ) ptr;
	int const x     = event->win_x;
	int const y     = event->win_y;

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

long forge::WorldViewport::on_face_flip( FXObject *, FXSelector, void * )
{
	ApeEditorInstance *instance = editor->get_internal();
	assert( instance != nullptr );

	ape_editor_flip_faces( instance );
	return TRUE;
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
		case ID_CREATE_NODE + APE_WORLD_NODE_TYPE_LIGHT:
		{
			static constexpr PLColourF32 DEFAULT_COLOUR = PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.0f );
			ape_create_light( APE_WORLD_NODE( room ), &tpos, &DEFAULT_COLOUR, 128.0f, APE_LIGHT_TYPE_OMNI, APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_SHADOWS );
			break;
		}
		case ID_CREATE_NODE + APE_WORLD_NODE_TYPE_CAMERA:
		{
			ape_create_camera( APE_WORLD_NODE( room ), "temp", &tpos, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
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

				ape_create_entity( cls->name, nullptr, APE_WORLD_NODE( room ) );
			}
			break;
		}
	}

	return TRUE;
}
