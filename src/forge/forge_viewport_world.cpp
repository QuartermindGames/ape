// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Forge world viewport implementation.
// Author:  Mark E. Sowden

#include "forge.h"
#include "forge_viewport_world.h"
#include "forge_editor_world.h"

FXIMPLEMENT( forge::WorldViewport, forge::Viewport, nullptr, 0 )

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

	switch ( instance->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_PLOT:
		{
			ape_editor_add_polygon_point( instance );
			return TRUE;
		}
		case APE_EDITOR_GEOMETRY_MODE_FACE: break;
		case APE_EDITOR_GEOMETRY_MODE_VERTEX: break;
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM: break;
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

	switch ( instance->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_PLOT:
		{
			ape_editor_brush_from_polygon( instance );
			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_FACE: break;
		case APE_EDITOR_GEOMETRY_MODE_VERTEX: break;
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

	auto *event = ( FXEvent * ) ptr;
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
		case KEY_KP_Subtract:
		{
			ape_grid_decrease_size();
			return TRUE;
		}
		case KEY_KP_Add:
		{
			ape_grid_increase_size();
			return TRUE;
		}

		// for testing
		case KEY_r:
		{
			if ( instance->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
			{
				break;
			}

			if ( instance->selectedFace == nullptr )
			{
				break;
			}

			ape_grid_align_to_face( &instance->grid, instance->selectedFace );
			return TRUE;
		}
		case KEY_n:
		{
			ApeRoom *room = ape_camera_get_room( camera );
			assert( room != nullptr );

			PLVector3 pos;
			if ( ape_grid_get_cursor_position( &instance->grid, ( PLVector2 * ) &pos ) == nullptr )
			{
				return TRUE;
			}

			pos = ape_grid_transform_point( &instance->grid, ( PLVector2 * ) &pos );

			PLColourF32 colour = PL_COLOURF32( PlGenerateRandomFloat( 1.0f ),
			                                   PlGenerateRandomFloat( 1.0f ),
			                                   PlGenerateRandomFloat( 1.0f ), 1.0f );

			ape_create_light( ( ApeWorldNode * ) room, &pos,
			                  &colour, 16.0f,
			                  APE_LIGHT_TYPE_OMNI,
			                  APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC | APE_LIGHT_FLAG_RUNTIME_SHADOWS );
		}
	}

	return Viewport::on_key( object, selector, ptr );
}