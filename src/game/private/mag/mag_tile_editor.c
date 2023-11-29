// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: 2D tile editor for MAG project.
// Author:  Mark E. Sowden

#include "mag_game.h"
#include "mag_world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static const char *worldLayerLabels[ MAG_WORLD_MAX_LAYERS ] = {
        [MAG_WORLD_LAYER_BACKGROUND] = "background",
        [MAG_WORLD_LAYER_SPRITES] = "sprites",
        [MAG_WORLD_LAYER_FOREGROUND] = "foreground",
};

static MagTileEditorStatus status = MAG_TILE_EDITOR_STATUS_CLOSED;

static void setup_editor( void )
{
}

static void mag_tile_editor_command( unsigned int, char ** )
{
	if ( status == MAG_TILE_EDITOR_STATUS_CLOSING )
	{
		Game_Warning( "Can't toggle editor while editor is still closing!\n" );
		return;
	}

	status = ( status == MAG_TILE_EDITOR_STATUS_OPEN ) ? MAG_TILE_EDITOR_STATUS_CLOSING : MAG_TILE_EDITOR_STATUS_OPEN;
	if ( status == MAG_TILE_EDITOR_STATUS_OPEN )
		setup_editor();
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void mag_tile_editor_initialize( void )
{
	PlRegisterConsoleCommand( "mag_tile_editor", "Toggle the MAG tile editor.", 0, mag_tile_editor_command );
}

MagTileEditorStatus mag_tile_editor_get_status( void )
{
	return status;
}

void mag_tile_editor_draw( SSArlViewport *viewport )
{
}

void mag_tile_editor_tick( void )
{
}
