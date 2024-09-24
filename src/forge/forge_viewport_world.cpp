// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Forge world viewport implementation.
// Author:  Mark E. Sowden

#include "forge.h"
#include "forge_viewport_world.h"
#include "editors/editor_world.h"

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
			PLVector3 pos;
			if ( ape_grid_get_cursor_position( &instance->grid, &pos ) == nullptr )
			{
				return FALSE;
			}

			ape_editor_add_polygon_point( editor->get_internal() );
			return TRUE;
		}
		case APE_EDITOR_GEOMETRY_MODE_FACE: break;
		//case APE_EDITOR_GEOMETRY_MODE_EDGE: break;
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
		//case APE_EDITOR_GEOMETRY_MODE_EDGE: break;
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
	}

	return Viewport::on_key( object, selector, ptr );
}