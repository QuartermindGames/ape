/*
===========================================================================
Copyright (C) 1997-2006 Id Software, Inc.

This file is part of Quake 2 Tools source code.

Quake 2 Tools source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake 2 Tools source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake 2 Tools source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// map.c

#include "qe3.h"

qboolean	modified;		// for quit confirmation (0 = clean, 1 = unsaved,
							// 2 = autosaved, but not regular saved)

char		currentmap[ 1024 ];

Brush	active_brushes;		// brushes currently being displayed
Brush	selected_brushes;	// highlighted
face_t *selected_face;
Brush *selected_face_brush;
Brush	filtered_brushes;	// brushes that have been filtered or regioned

entity_t	entities;		// head/tail of doubly linked list

entity_t *world_entity;

void AddRegionBrushes( void );
void RemoveRegionBrushes( void );

/*
=============================================================

  Cross map selection saving

  this could fuck up if you have only part of a complex entity selected...
=============================================================
*/

Brush		between_brushes;
entity_t	between_entities;


void Map_SaveBetween( void ) {
	Brush *b;
	entity_t *e, *e2;

	between_brushes.next = selected_brushes.next;
	between_brushes.prev = selected_brushes.prev;
	between_brushes.next->prev = &between_brushes;
	between_brushes.prev->next = &between_brushes;

	between_entities.next = between_entities.prev = &between_entities;
	selected_brushes.next = selected_brushes.prev = &selected_brushes;

	for( b = between_brushes.next; b != &between_brushes; b = b->next ) {
		e = b->owner;
		if( e == world_entity )
			b->owner = NULL;
		else {
			for( e2 = between_entities.next; e2 != &between_entities; e2 = e2->next )
				if( e2 == e )
					goto next;	// allready got the entity
			// move the entity over
			e->prev->next = e->next;
			e->next->prev = e->prev;
			e->next = between_entities.next;
			e->prev = &between_entities;
			e->next->prev = e;
			e->prev->next = e;
		}
	next:;
	}
}

void Map_RestoreBetween( void ) {
	entity_t *head, *tail;
	Brush *b;

	if( !between_brushes.next )
		return;

	for( b = between_brushes.next; b != &between_brushes; b = b->next ) {
		if( !b->owner ) {
			b->owner = world_entity;
			b->onext = world_entity->brushes.onext;
			b->oprev = &world_entity->brushes;
			b->onext->oprev = b;
			b->oprev->onext = b;
		}
	}

	selected_brushes.next = between_brushes.next;
	selected_brushes.prev = between_brushes.prev;
	selected_brushes.next->prev = &selected_brushes;
	selected_brushes.prev->next = &selected_brushes;

	head = between_entities.next;
	tail = between_entities.prev;

	if( head != tail ) {
		entities.prev->next = head;
		head->prev = entities.prev;
		tail->next = &entities;
		entities.prev = tail;
	}

	between_brushes.next = NULL;
	between_entities.next = NULL;
}

//============================================================================

void Map_BuildBrushData( void ) {
	Brush *b, *next;

	if( active_brushes.next == NULL )
		return;

	Sys_BeginWait();	// this could take a while

	for( b = active_brushes.next; b != NULL && b != &active_brushes; b = next ) {
		next = b->next;
		b->Build();
		if( !b->brush_faces ) {
			delete b;
			Sys_Printf( "Removed degenerate brush\n" );
		}
	}

	Sys_EndWait();
}

entity_t *Map_FindClass( const char *cname ) {
	entity_t *ent;

	for( ent = entities.next; ent != &entities; ent = ent->next ) {
		if( !strcmp( cname, ValueForKey( ent, "classname" ) ) )
			return ent;
	}
	return nullptr;
}

/*
================
Map_Free
================
*/
void Map_Free( void ) {
	if( selected_brushes.next &&
		( selected_brushes.next != &selected_brushes ) ) {
		if( MessageBox( g_qeglobals.d_hwndMain, "Copy selection?", "", MB_YESNO ) == IDYES )
			Map_SaveBetween();
	}

	Texture_ClearInuse();
	Pointfile_Clear();
	strcpy( currentmap, "unnamed.map" );
	Sys_SetTitle( currentmap );
	g_qeglobals.d_num_entities = 0;

	if( !active_brushes.next ) {	// first map
		active_brushes.prev = active_brushes.next = &active_brushes;
		selected_brushes.prev = selected_brushes.next = &selected_brushes;
		filtered_brushes.prev = filtered_brushes.next = &filtered_brushes;

		entities.prev = entities.next = &entities;
	} else {
		while( active_brushes.next != &active_brushes )
			delete ( active_brushes.next );
		while( selected_brushes.next != &selected_brushes )
			delete ( selected_brushes.next );
		while( filtered_brushes.next != &filtered_brushes )
			delete ( filtered_brushes.next );

		while( entities.next != &entities )
			Entity_Free( entities.next );
	}

	world_entity = NULL;
}

/*
================
Map_LoadFile
================
*/
void Map_LoadFile( const char *filename ) {
	char *buf;
	entity_t *ent;
	char         temp[ 1024 ];

	Sys_BeginWait();

	SetInspectorMode( W_CONSOLE );

	QE_ConvertDOSToUnixName( temp, filename );
	Sys_Printf( "Map_LoadFile: %s\n", temp );

	Map_Free();

	g_qeglobals.d_parsed_brushes = 0;
	strcpy( currentmap, filename );
	LoadFile( filename, (void **)&buf );

	StartTokenParsing( buf );

	g_qeglobals.d_num_entities = 0;

	while( 1 ) {
		ent = Entity_Parse( false );
		if( !ent )
			break;
		if( !strcmp( ValueForKey( ent, "classname" ), "worldspawn" ) ) {
			if( world_entity )
				Sys_Printf( "WARNING: multiple worldspawn\n" );
			world_entity = ent;
		} else {
			// add the entity to the end of the entity list
			ent->next = &entities;
			ent->prev = entities.prev;
			entities.prev->next = ent;
			entities.prev = ent;
			g_qeglobals.d_num_entities++;
		}
	}

	free( buf );

	if( !world_entity ) {
		Sys_Printf( "No worldspawn in map.\n" );
		Map_New();
		return;
	}

	Sys_Printf( "--- LoadMapFile ---\n" );
	Sys_Printf( "%s\n", temp );

	Sys_Printf( "%5i brushes\n", g_qeglobals.d_parsed_brushes );
	Sys_Printf( "%5i entities\n", g_qeglobals.d_num_entities );

	Map_RestoreBetween();

	Sys_Printf( "Map_BuildAllDisplayLists\n" );
	Map_BuildBrushData();

	g_mainWindow->ResetViews();

	Sys_UpdateWindows( W_ALL );

	Map_RegionOff();

	modified = false;
	Sys_SetTitle( temp );

	Texture_ShowInuse();

	Sys_EndWait();

}

/*
===========
Map_SaveFile
===========
*/
void Map_SaveFile( const char *filename, qboolean use_region ) {
	entity_t *e, *next;
	FILE *f;
	char         temp[ 1024 ];
	int			count;

	// Fallback to unnamed.map if one doesn't exist
	if( filename == nullptr ) {
		// TODO: save into user dir!
		filename = "unnamed.map";
	}

	QE_ConvertDOSToUnixName( temp, filename );

	if( !use_region ) {
		char	backup[ 1024 ];

		// rename current to .bak
		strncpy( backup, filename, sizeof( backup ) - 4 );
		StripExtension( backup );
		strcat( backup, ".bak" );
		_unlink( backup );
		rename( filename, backup );
	}

	Sys_Printf( "Map_SaveFile: %s\n", filename );

	f = fopen( filename, "w" );
	if( !f ) {
		Sys_Printf( "ERROR!!!! Couldn't open %s\n", filename );
		return;
	}

	if( use_region )
		AddRegionBrushes();

	// write world entity first
	Entity_Write( world_entity, f, use_region );

	// then write all other ents
	count = 1;
	for( e = entities.next; e != &entities; e = next ) {
		fprintf( f, "// entity %i\n", count );
		count++;
		next = e->next;
		if( e->brushes.onext == &e->brushes )
			Entity_Free( e );	// no brushes left, so remove it
		else
			Entity_Write( e, f, use_region );
	}

	fclose( f );

	if( use_region )
		RemoveRegionBrushes();

	Sys_Printf( "Saved.\n" );
	modified = false;

	if( !strstr( temp, "autosave" ) )
		Sys_SetTitle( temp );
}

/*
===========
Map_New
===========
*/
void Map_New( void ) {
	Sys_Printf( "Map_New\n" );
	Map_Free();
	world_entity = (entity_t *)qmalloc( sizeof( *world_entity ) );
	world_entity->brushes.onext =
		world_entity->brushes.oprev = &world_entity->brushes;
	SetKeyValue( world_entity, "classname", "worldspawn" );
	world_entity->eclass = Eclass_ForName( "worldspawn", true );

	g_mainWindow->ResetViews();

	Map_RestoreBetween();

	Sys_UpdateWindows( W_ALL );
	modified = false;
}


/*
===========================================================

  REGION

===========================================================
*/

qboolean	region_active;
vec3_t	region_mins = { -4096, -4096, -4096 };
vec3_t	region_maxs = { 4096, 4096, 4096 };

Brush *region_sides[ 4 ];

/*
===========
AddRegionBrushes

a regioned map will have temp walls put up at the region boundary
===========
*/
void AddRegionBrushes( void ) {
	vec3_t	mins, maxs;
	int		i;
	texdef_t	td;

	if( !region_active )
		return;

	memset( &td, 0, sizeof( td ) );
	strcpy( td.name, "REGION" );

	mins[ 0 ] = region_mins[ 0 ] - 16;
	maxs[ 0 ] = region_mins[ 0 ] + 1;
	mins[ 1 ] = region_mins[ 1 ] - 16;
	maxs[ 1 ] = region_maxs[ 1 ] + 16;
	mins[ 2 ] = -2048;
	maxs[ 2 ] = 2048;
	region_sides[ 0 ] = new Brush( mins, maxs, &td );

	mins[ 0 ] = region_maxs[ 0 ] - 1;
	maxs[ 0 ] = region_maxs[ 0 ] + 16;
	region_sides[ 1 ] = new Brush( mins, maxs, &td );

	mins[ 0 ] = region_mins[ 0 ] - 16;
	maxs[ 0 ] = region_maxs[ 0 ] + 16;
	mins[ 1 ] = region_mins[ 1 ] - 16;
	maxs[ 1 ] = region_mins[ 1 ] + 1;
	region_sides[ 2 ] = new Brush( mins, maxs, &td );

	mins[ 1 ] = region_maxs[ 1 ] - 1;
	maxs[ 1 ] = region_maxs[ 1 ] + 16;
	region_sides[ 3 ] = new Brush( mins, maxs, &td );

	for( i = 0; i < 4; i++ ) {
		region_sides[ i ]->AddToList( &selected_brushes );
		Entity_LinkBrush( world_entity, region_sides[ i ] );
		region_sides[ i ]->Build();
	}
}

void RemoveRegionBrushes( void ) {
	int		i;

	if( !region_active )
		return;
	for( i = 0; i < 4; i++ )
		delete ( region_sides[ i ] );
}


qboolean Map_IsBrushFiltered( Brush *b ) {
	int		i;

	for( i = 0; i < 3; i++ ) {
		if( b->mins[ i ] > region_maxs[ i ] )
			return true;
		if( b->maxs[ i ] < region_mins[ i ] )
			return true;
	}
	return false;
}

/*
===========
Map_RegionOff

Other filtering options may still be on
===========
*/
void Map_RegionOff( void ) {
	Brush *b, *next;
	int			i;

	region_active = false;
	for( i = 0; i < 3; i++ ) {
		region_maxs[ i ] = 4096;
		region_mins[ i ] = -4096;
	}

	for( b = filtered_brushes.next; b != &filtered_brushes; b = next ) {
		next = b->next;
		if( Map_IsBrushFiltered( b ) )
			continue;		// still filtered
		b->RemoveFromList();
		b->AddToList( &active_brushes );
	}

	Sys_UpdateWindows( W_ALL );
}

void Map_ApplyRegion( void ) {
	Brush *b, *next;

	region_active = true;
	for( b = active_brushes.next; b != &active_brushes; b = next ) {
		next = b->next;
		if( !Map_IsBrushFiltered( b ) )
			continue;		// still filtered
		b->RemoveFromList();
		b->AddToList( &filtered_brushes );
	}

	Sys_UpdateWindows( W_ALL );
}


/*
========================
Map_RegionSelectedBrushes
========================
*/
void Map_RegionSelectedBrushes( void ) {
	Map_RegionOff();

	region_active = true;
	Select_GetBounds( region_mins, region_maxs );

	// move the entire active_brushes list to filtered_brushes
	filtered_brushes.next = active_brushes.next;
	filtered_brushes.prev = active_brushes.prev;
	filtered_brushes.next->prev = &filtered_brushes;
	filtered_brushes.prev->next = &filtered_brushes;

	// move the entire selected_brushes list to active_brushes
	active_brushes.next = selected_brushes.next;
	active_brushes.prev = selected_brushes.prev;
	active_brushes.next->prev = &active_brushes;
	active_brushes.prev->next = &active_brushes;

	// clear selected_brushes
	selected_brushes.next = selected_brushes.prev = &selected_brushes;

	Sys_UpdateWindows( W_ALL );
}


/*
===========
Map_RegionXY
===========
*/
void Map_RegionXY( void ) {
	Map_RegionOff();
#if 0 // todo
	region_mins[ 0 ] = g_qeglobals.d_xy.origin[ 0 ] - 0.5 * g_qeglobals.d_xy.width / g_qeglobals.d_xy.scale;
	region_maxs[ 0 ] = g_qeglobals.d_xy.origin[ 0 ] + 0.5 * g_qeglobals.d_xy.width / g_qeglobals.d_xy.scale;
	region_mins[ 1 ] = g_qeglobals.d_xy.origin[ 1 ] - 0.5 * g_qeglobals.d_xy.height / g_qeglobals.d_xy.scale;
	region_maxs[ 1 ] = g_qeglobals.d_xy.origin[ 1 ] + 0.5 * g_qeglobals.d_xy.height / g_qeglobals.d_xy.scale;
#endif
	region_mins[ 2 ] = -4096;
	region_maxs[ 2 ] = 4096;

	Map_ApplyRegion();
}

/*
===========
Map_RegionTallBrush
===========
*/
void Map_RegionTallBrush( void ) {
	Brush *b;

	if( !QE_SingleBrush() )
		return;

	b = selected_brushes.next;

	Map_RegionOff();

	VectorCopy( b->mins, region_mins );
	VectorCopy( b->maxs, region_maxs );
	region_mins[ 2 ] = -4096;
	region_maxs[ 2 ] = 4096;

	Select_Delete();
	Map_ApplyRegion();
}
/*
===========
Map_RegionBrush
===========
*/
void Map_RegionBrush( void ) {
	Brush *b;

	if( !QE_SingleBrush() )
		return;

	b = selected_brushes.next;

	Map_RegionOff();

	VectorCopy( b->mins, region_mins );
	VectorCopy( b->maxs, region_maxs );

	Select_Delete();
	Map_ApplyRegion();
}

