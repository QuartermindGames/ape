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

#include "qe3.h"
#include "Viewport.h"
#include "xy.h"

#define PAGEFLIPS 2

huang::XYZPerspective::XYZPerspective( Viewport *viewport ) : BasePerspective( viewport ) {}
huang::XYZPerspective::~XYZPerspective() = default;

/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

//static	int	buttonstate;
static int      pressx, pressy;
static vec3_t   pressdelta;
static qboolean press_selection;

void huang::XYZPerspective::ToPoint( int x, int y, vec3_t point )
{
	int width  = parentViewport->GetCanvasWidth();
	int height = parentViewport->GetCanvasHeight();
	point[ 0 ] = origin[ 0 ] + ( x - width / 2 ) / scale;
	point[ 1 ] = origin[ 1 ] + ( y - height / 2 ) / scale;
	point[ 2 ] = 0.0f;
}

void huang::XYZPerspective::ToGridPoint( int x, int y, vec3_t point )
{
	int width  = parentViewport->GetCanvasWidth();
	int height = parentViewport->GetCanvasHeight();
	point[ 0 ] = origin[ 0 ] + ( x - width / 2 ) / scale;
	point[ 1 ] = origin[ 1 ] + ( y - height / 2 ) / scale;
	point[ 2 ] = 0;
	point[ 0 ] = floor( point[ 0 ] / g_qeglobals.d_gridsize + 0.5 ) * g_qeglobals.d_gridsize;
	point[ 1 ] = floor( point[ 1 ] / g_qeglobals.d_gridsize + 0.5 ) * g_qeglobals.d_gridsize;
}

/*
==============
XY_MouseDown
==============
*/
void huang::XYZPerspective::MouseDown( int x, int y, const bool buttons[] )
{
	vec3_t point;
	vec3_t origin, dir, right, up;

	pressx = x;
	pressy = y;
	VectorCopy( vec3_origin, pressdelta );

	ToPoint( x, y, point );

	VectorCopy( point, origin );
	origin[ 2 ] = 8192.0f;

	dir[ 0 ]   = 0;
	dir[ 1 ]   = 0;
	dir[ 2 ]   = -1;
	right[ 0 ] = 1 / scale;
	right[ 1 ] = 0;
	right[ 2 ] = 0;
	up[ 0 ]    = 0;
	up[ 1 ]    = 1 / scale;
	up[ 2 ]    = 0;

	press_selection = ( selected_brushes.next != &selected_brushes );

	FXuint temp;
	g_mainWindow->getCursorPosition( dragX, dragY, temp );

	if ( buttons[ huang::input::MOUSE_BUTTON_LEFT ] )
	{
		if ( buttons[ huang::input::BUTTON_MOD_SHIFT ] )
		{
			return;
		}

		Drag_Begin( x, y, buttons, right, up, origin, dir );
		return;
	}

	CameraPerspective *camera = parentViewport->GetCamera();
	if ( camera == nullptr )
	{
		return;
	}

	// control mbutton = move camera
	if ( buttons[ huang::input::MOUSE_BUTTON_RIGHT ] && buttons[ huang::input::BUTTON_MOD_CONTROL ] )
	{
		camera->SetPosition( point );
	}
	else if ( buttons[ huang::input::MOUSE_BUTTON_RIGHT ] && buttons[ huang::input::BUTTON_MOD_SHIFT ] )
	{
		VectorSubtract( point, camera->origin, point );
		if ( point[ 1 ] > 0.0f || point[ 0 ] > 0.0f )
		{
			camera->angles[ YAW ] = 180.0f / Q_PIF * atan2f( point[ 1 ], point[ 0 ] );
		}
	}
}

/*
==============
XY_MouseUp
==============
*/
void huang::XYZPerspective::MouseUp( int x, int y, const bool buttons[] )
{
	Drag_MouseUp();

	if ( !press_selection )
		Sys_UpdateWindows( W_ALL );
}

bool huang::XYZPerspective::DragDelta( int x, int y, vec3_t move )
{
	vec3_t xvec, yvec, delta;
	int    i;

	xvec[ 0 ] = 1.0f / scale;
	xvec[ 1 ] = xvec[ 2 ] = 0;
	yvec[ 1 ]             = 1.0f / scale;
	yvec[ 0 ] = yvec[ 2 ] = 0;

	for ( i = 0; i < 3; i++ )
	{
		delta[ i ] = xvec[ i ] * ( x - pressx ) + yvec[ i ] * ( y - pressy );
		delta[ i ] = floor( delta[ i ] / g_qeglobals.d_gridsize + 0.5 ) * g_qeglobals.d_gridsize;
	}
	VectorSubtract( delta, pressdelta, move );
	VectorCopy( delta, pressdelta );

	return ( move[ 0 ] > 0.0f || move[ 1 ] > 0.0f || move[ 2 ] > 0.0f );
}

/*
==============
NewBrushDrag
==============
*/
void huang::XYZPerspective::NewBrushDrag( int x, int y )
{
	vec3_t mins, maxs, junk;
	int    i;
	float  temp;
	Brush *n;

	if ( !DragDelta( x, y, junk ) )
		return;
	// delete the current selection
	if ( selected_brushes.next != &selected_brushes )
		delete ( selected_brushes.next );
	ToGridPoint( pressx, pressy, mins );
	mins[ 2 ] = g_qeglobals.d_gridsize * ( ( int ) ( g_qeglobals.d_new_brush_bottom_z / g_qeglobals.d_gridsize ) );
	ToGridPoint( x, y, maxs );
	maxs[ 2 ] = g_qeglobals.d_gridsize * ( ( int ) ( g_qeglobals.d_new_brush_top_z / g_qeglobals.d_gridsize ) );
	if ( maxs[ 2 ] <= mins[ 2 ] )
		maxs[ 2 ] = mins[ 2 ] + g_qeglobals.d_gridsize;

	for ( i = 0; i < 3; i++ )
	{
		if ( mins[ i ] == maxs[ i ] )
			return;// don't create a degenerate brush
		if ( mins[ i ] > maxs[ i ] )
		{
			temp      = mins[ i ];
			mins[ i ] = maxs[ i ];
			maxs[ i ] = temp;
		}
	}

	n = new Brush( mins, maxs, &g_qeglobals.d_texturewin.texdef );
	n->AddToList( &selected_brushes );

	Entity_LinkBrush( world_entity, n );

	n->Build();
}

void huang::XYZPerspective::MouseMoved( int x, int y, const bool buttons[] )
{
	lastX = x;
	lastY = y;

	World *world = g_mainWindow->GetCurrentWorld();
	if ( world == nullptr )
	{
		return;
	}

	switch ( g_mainWindow->GetEditMode() )
	{
		case EDIT_MODE_VERTEX:
			// lbutton without selection = drag new brush
			if ( buttons[ huang::input::MOUSE_BUTTON_LEFT ] && !press_selection )
			{
				vec3_t pos;
				ToGridPoint( x, y, pos );

				PLGVertex vertex = PlgInitializeVertex();
				switch ( parentViewport->GetViewMode() )
				{
					case VIEW_MODE_FRONT:
						vertex.position.x = pos[ 0 ];
						vertex.position.y = pos[ 1 ];
						break;
					case VIEW_MODE_LEFT:
						vertex.position.x = pos[ 0 ];
						vertex.position.y = pos[ 1 ];
						break;
					case VIEW_MODE_TOP:
						vertex.position.x = pos[ 0 ];
						vertex.position.y = pos[ 1 ];
						break;
					default:
						break;
				}

				world->vertices.push_back( vertex );
				return;
			}
			break;
		case EDIT_MODE_BRUSH:
			// lbutton without selection = drag new brush
			if ( buttons[ huang::input::MOUSE_BUTTON_LEFT ] && !press_selection )
			{
				NewBrushDrag( x, y );
				return;
			}
			break;
		default:
			break;
	}

	// lbutton (possibly with control and or shift)
	// with selection = drag selection
	if ( buttons[ huang::input::MOUSE_BUTTON_LEFT ] )
	{
		Drag_MouseMoved( x, y, buttons );
		return;
	}

	// rbutton = drag xy origin
	if ( buttons[ huang::input::MOUSE_BUTTON_RIGHT ] )
	{
		FXuint buttons;
		g_mainWindow->getCursorPosition( x, y, buttons );
		if ( x != dragX || y != dragY )
		{
			origin[ 0 ] -= ( x - dragX ) / scale;
			origin[ 1 ] += ( y - dragY ) / scale;
			g_mainWindow->setCursorPosition( dragX, dragY );
		}
		return;
	}

#if 0// TODO
	vec3_t	point;
	// control mbutton = move camera
	if( buttonstate == ( MK_CONTROL | MK_MBUTTON ) ) {
		XY_ToPoint( x, y, point );
		camera.origin[ 0 ] = point[ 0 ];
		camera.origin[ 1 ] = point[ 1 ];
		Sys_UpdateWindows( W_XY_OVERLAY | W_CAMERA );
		return;
	}

	// mbutton = angle camera
	if( buttonstate == MK_MBUTTON ) {
		XY_ToPoint( x, y, point );
		VectorSubtract( point, camera.origin, point );
		if( point[ 1 ] || point[ 0 ] ) {
			camera.angles[ YAW ] = 180 / Q_PI * atan2( point[ 1 ], point[ 0 ] );
			Sys_UpdateWindows( W_XY_OVERLAY | W_CAMERA );
		}
		return;
	}
#endif
}


/*
============================================================================

DRAWING

============================================================================
*/


/*
==============
XY_DrawGrid
==============
*/
void huang::XYZPerspective::DrawGrid()
{
	float x, y, xb, xe, yb, ye;
	int   w, h;

	glDisable( GL_TEXTURE_2D );
	glDisable( GL_TEXTURE_1D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );

	w = parentViewport->GetCanvasWidth() / 2 / scale;
	h = parentViewport->GetCanvasHeight() / 2 / scale;

	xb = origin[ 0 ] - w;
	if ( xb < region_mins[ 0 ] )
		xb = region_mins[ 0 ];
	xb = 64 * floor( xb / 64 );

	xe = origin[ 0 ] + w;
	if ( xe > region_maxs[ 0 ] )
		xe = region_maxs[ 0 ];
	xe = 64 * ceil( xe / 64 );

	yb = origin[ 1 ] - h;
	if ( yb < region_mins[ 1 ] )
		yb = region_mins[ 1 ];
	yb = 64 * floor( yb / 64 );

	ye = origin[ 1 ] + h;
	if ( ye > region_maxs[ 1 ] )
		ye = region_maxs[ 1 ];
	ye = 64 * ceil( ye / 64 );

	// draw major blocks

	glColor3fv( g_qeglobals.d_savedinfo.colors[ COLOR_GRIDMAJOR ] );

	if ( g_qeglobals.d_showgrid )
	{

		glBegin( GL_LINES );

		for ( x = xb; x <= xe; x += 64 )
		{
			glVertex2f( x, yb );
			glVertex2f( x, ye );
		}
		for ( y = yb; y <= ye; y += 64 )
		{
			glVertex2f( xb, y );
			glVertex2f( xe, y );
		}

		glEnd();
	}

	// draw minor blocks
	if ( g_qeglobals.d_showgrid && g_qeglobals.d_gridsize * scale >= 4 )
	{
		glColor3fv( g_qeglobals.d_savedinfo.colors[ COLOR_GRIDMINOR ] );

		glBegin( GL_LINES );
		for ( x = xb; x < xe; x += g_qeglobals.d_gridsize )
		{
			if ( !( ( int ) x & 63 ) )
				continue;
			glVertex2f( x, yb );
			glVertex2f( x, ye );
		}
		for ( y = yb; y < ye; y += g_qeglobals.d_gridsize )
		{
			if ( !( ( int ) y & 63 ) )
				continue;
			glVertex2f( xb, y );
			glVertex2f( xe, y );
		}
		glEnd();
	}

	// draw coordinate text if needed
#if 0// todo: redo!
	if( g_qeglobals.d_savedinfo.show_coordinates ) {
		glColor4f( 0, 0, 0, 0 );

		char	text[ 32 ];
		for( x = xb; x < xe; x += 64 ) {
			glRasterPos2f( x, g_qeglobals.d_xy.origin[ 1 ] + h - 6 / g_qeglobals.d_xy.scale );
			sprintf( text, "%i", (int)x );
			glCallLists( strlen( text ), GL_UNSIGNED_BYTE, text );
		}
		for( y = yb; y < ye; y += 64 ) {
			glRasterPos2f( g_qeglobals.d_xy.origin[ 0 ] - w + 1, y );
			sprintf( text, "%i", (int)y );
			glCallLists( strlen( text ), GL_UNSIGNED_BYTE, text );
		}
	}
#endif
}

/*
==============
XY_DrawBlockGrid
==============
*/
void huang::XYZPerspective::DrawBlockGrid()
{
	float x, y, xb, xe, yb, ye;
	int   w, h;
	char  text[ 32 ];

	glDisable( GL_TEXTURE_2D );
	glDisable( GL_TEXTURE_1D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );

	w = parentViewport->GetCanvasWidth() / 2 / scale;
	h = parentViewport->GetCanvasHeight() / 2 / scale;

	xb = origin[ 0 ] - w;
	if ( xb < region_mins[ 0 ] )
		xb = region_mins[ 0 ];
	xb = 1024 * floor( xb / 1024 );

	xe = origin[ 0 ] + w;
	if ( xe > region_maxs[ 0 ] )
		xe = region_maxs[ 0 ];
	xe = 1024 * ceil( xe / 1024 );

	yb = origin[ 1 ] - h;
	if ( yb < region_mins[ 1 ] )
		yb = region_mins[ 1 ];
	yb = 1024 * floor( yb / 1024 );

	ye = origin[ 1 ] + h;
	if ( ye > region_maxs[ 1 ] )
		ye = region_maxs[ 1 ];
	ye = 1024 * ceil( ye / 1024 );

	// draw major blocks

	glColor3f( 0, 0, 1 );
	glLineWidth( 2 );

	glBegin( GL_LINES );

	for ( x = xb; x <= xe; x += 1024 )
	{
		glVertex2f( x, yb );
		glVertex2f( x, ye );
	}
	for ( y = yb; y <= ye; y += 1024 )
	{
		glVertex2f( xb, y );
		glVertex2f( xe, y );
	}

	glEnd();
	glLineWidth( 1 );

	// draw coordinate text if needed

	for ( x = xb; x < xe; x += 1024 )
		for ( y = yb; y < ye; y += 1024 )
		{
			glRasterPos2f( x + 512, y + 512 );
			sprintf( text, "%i,%i", ( int ) floor( x / 1024 ), ( int ) floor( y / 1024 ) );
			glCallLists( ( GLsizei ) strlen( text ), GL_UNSIGNED_BYTE, text );
		}

	glColor4f( 0, 0, 0, 0 );
}

bool FilterBrush( const Brush *pb )
{
	if ( !pb->owner )
		return FALSE;// during construction

	if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_CLIP )
	{
		if ( !strncmp( pb->brush_faces->texdef.name, "clip", 4 ) )
			return TRUE;
	}

	if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WATER )
	{
		if ( pb->brush_faces->texdef.name[ 0 ] == '*' )
			return TRUE;
	}

	if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_DETAIL )
	{
		if ( pb->brush_faces->texdef.contents & CONTENTS_DETAIL )
			return TRUE;
	}

	if ( pb->owner == world_entity )
	{
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WORLD )
			return TRUE;
		return FALSE;
	}
	else if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_ENT )
		return TRUE;

	if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_LIGHTS )
	{
		if ( !strncmp( pb->owner->eclass->name, "light", 5 ) )
			return TRUE;
	}

	if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS )
	{
		if ( !strncmp( pb->owner->eclass->name, "path", 4 ) )
			return TRUE;
	}

	return FALSE;
}

/*
=============================================================

  PATH LINES

=============================================================
*/

/*
==================
DrawPathLines

Draws connections between entities.
Needs to consider all entities, not just ones on screen,
because the lines can be visible when neither end is.
Called for both camera view and xy view.
==================
*/
void DrawPathLines()
{
	int         i, j, k;
	vec3_t      mid, mid1;
	entity_t *  se, *te;
	Brush *     sb, *tb;
	vec3_t      dir, s1, s2;
	vec_t       len, f;
	int         arrows;
	int         num_entities;
	const char *ent_target[ MAX_MAP_ENTITIES ];
	entity_t *  ent_entity[ MAX_MAP_ENTITIES ];


	num_entities = 0;
	for ( te = entities.next; te != &entities && num_entities != MAX_MAP_ENTITIES; te = te->next )
	{
		ent_target[ num_entities ] = ValueForKey( te, "target" );
		if ( ent_target[ num_entities ][ 0 ] )
		{
			ent_entity[ num_entities ] = te;
			num_entities++;
		}
	}

	for ( se = entities.next; se != &entities; se = se->next )
	{
		const char *psz = ValueForKey( se, "targetname" );
		if ( psz == NULL || psz[ 0 ] == '\0' )
			continue;

		sb = se->brushes.onext;
		if ( sb == &se->brushes )
			continue;

		for ( k = 0; k < num_entities; k++ )
		{
			if ( strcmp( ent_target[ k ], psz ) )
				continue;

			te = ent_entity[ k ];
			tb = te->brushes.onext;
			if ( tb == &te->brushes )
				continue;

			for ( i = 0; i < 3; i++ )
				mid[ i ] = ( sb->mins[ i ] + sb->maxs[ i ] ) * 0.5f;

			for ( i = 0; i < 3; i++ )
				mid1[ i ] = ( tb->mins[ i ] + tb->maxs[ i ] ) * 0.5f;

			VectorSubtract( mid1, mid, dir );
			len     = VectorNormalize( dir );
			s1[ 0 ] = -dir[ 1 ] * 8 + dir[ 0 ] * 8;
			s2[ 0 ] = dir[ 1 ] * 8 + dir[ 0 ] * 8;
			s1[ 1 ] = dir[ 0 ] * 8 + dir[ 1 ] * 8;
			s2[ 1 ] = -dir[ 0 ] * 8 + dir[ 1 ] * 8;

			glColor3f( se->eclass->color[ 0 ], se->eclass->color[ 1 ], se->eclass->color[ 2 ] );

			glBegin( GL_LINES );
			glVertex3fv( mid );
			glVertex3fv( mid1 );

			arrows = ( int ) ( len / 256 ) + 1;

			for ( i = 0; i < arrows; i++ )
			{
				f = len * ( i + 0.5 ) / arrows;

				for ( j = 0; j < 3; j++ )
					mid1[ j ] = mid[ j ] + f * dir[ j ];
				glVertex3fv( mid1 );
				glVertex3f( mid1[ 0 ] + s1[ 0 ], mid1[ 1 ] + s1[ 1 ], mid1[ 2 ] );
				glVertex3fv( mid1 );
				glVertex3f( mid1[ 0 ] + s2[ 0 ], mid1[ 1 ] + s2[ 1 ], mid1[ 2 ] );
			}

			glEnd();
		}
	}
}

//=============================================================


/*
==============
XY_Draw
==============
*/
void huang::XYZPerspective::Draw()
{
	Brush *   brush;
	float     w, h;
	entity_t *e;
	vec3_t    mins, maxs;
	int       drawn, culled;

	//
	// clear
	//
	d_dirty = false;

	glClearColor(
	        g_qeglobals.d_savedinfo.colors[ COLOR_GRIDBACK ][ 0 ],
	        g_qeglobals.d_savedinfo.colors[ COLOR_GRIDBACK ][ 1 ],
	        g_qeglobals.d_savedinfo.colors[ COLOR_GRIDBACK ][ 2 ],
	        0 );
	glClear( GL_COLOR_BUFFER_BIT );

	//
	// set up viewpoint
	//
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	w         = ( float ) parentViewport->GetCanvasWidth() / 2.0f / scale;
	h         = ( float ) parentViewport->GetCanvasHeight() / 2.0f / scale;
	mins[ 0 ] = origin[ 0 ] - w;
	maxs[ 0 ] = origin[ 0 ] + w;
	mins[ 1 ] = origin[ 1 ] - h;
	maxs[ 1 ] = origin[ 1 ] + h;

	glOrtho( mins[ 0 ], maxs[ 0 ], mins[ 1 ], maxs[ 1 ], -8000, 8000 );

	//
	// now draw the grid
	//
	DrawGrid();

	World *world = g_mainWindow->GetCurrentWorld();
	if ( world == nullptr )
	{
		return;
	}

	switch ( parentViewport->GetViewMode() )
	{
		case huang::VIEW_MODE_FRONT:
			glRotatef( -90, 1, 0, 0 );// put Z going up
			glRotatef( 90, 0, 0, 1 ); // put Z going up
			break;
		case huang::VIEW_MODE_LEFT:
			glRotatef( -90, 1, 0, 0 );// put Z going up
			break;
		default:
			break;
	}

	//
	// draw stuff
	//
	glShadeModel( GL_FLAT );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_TEXTURE_1D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	glColor3f( 0, 0, 0 );

	drawn = culled = 0;

	if ( !active_brushes.next )
		return;// not valid yet

	e = nullptr;
	for ( brush = active_brushes.next; brush != &active_brushes; brush = brush->next )
	{
		if ( brush->mins[ 0 ] > maxs[ 0 ] || brush->mins[ 1 ] > maxs[ 1 ] || brush->maxs[ 0 ] < mins[ 0 ] || brush->maxs[ 1 ] < mins[ 1 ] )
		{
			culled++;
			continue;// off screen
		}

		if ( FilterBrush( brush ) )
		{
			continue;
		}

		drawn++;
		if ( brush->owner != e )
		{
			e = brush->owner;
			glColor3fv( e->eclass->color );
		}
		brush->Draw( parentViewport );
	}

	DrawPathLines();

	//
	// draw pointfile
	//
	if ( g_qeglobals.d_pointfile_display_list )
		glCallList( g_qeglobals.d_pointfile_display_list );

	//
	// draw block grid
	//
	if ( g_qeglobals.show_blocks )
		DrawBlockGrid();

	//
	// now draw selected brushes
	//
	{
		glTranslatef( g_qeglobals.d_select_translate[ 0 ], g_qeglobals.d_select_translate[ 1 ], g_qeglobals.d_select_translate[ 2 ] );
		glColor3f( 1.0, 0.0, 0.0 );
		glLineWidth( 2 );
		for ( brush = selected_brushes.next; brush != &selected_brushes; brush = brush->next )
		{
			drawn++;
			brush->Draw( parentViewport );
		}
		glLineWidth( 1 );
	}

	if ( !world->IsSelectionEmpty() )
	{
		glPointSize( 4.0f );
		glColor3f( 1.0f, 0.0f, 0.0f );

		glBegin( GL_POINTS );
		for ( const auto &vertex : world->selectedVertices )
		{
			glVertex3f( vertex->position.x, vertex->position.y, vertex->position.z );
		}
		glEnd();

		glBegin( GL_LINES );
		for ( const auto &face : world->selectedFaces )
		{
		}
		glEnd();

		glPointSize( 1.0f );
	}

	// edge / vertex flags

	switch ( g_mainWindow->GetEditMode() )
	{
		case EDIT_MODE_EDGE:
		{
			glPointSize( 4 );
			glColor3f( 0, 0, 1 );
			glBegin( GL_POINTS );
			// Draw edge points
			for ( unsigned int i = 0; i < g_qeglobals.d_numedges; i++ )
			{
				float *v1 = g_qeglobals.d_points[ g_qeglobals.d_edges[ i ].p1 ];
				float *v2 = g_qeglobals.d_points[ g_qeglobals.d_edges[ i ].p2 ];
				glVertex3f( ( v1[ 0 ] + v2[ 0 ] ) * 0.5f, ( v1[ 1 ] + v2[ 1 ] ) * 0.5f, ( v1[ 2 ] + v2[ 2 ] ) * 0.5f );
			}
			// Draw vertices
			glPointSize( 2 );
			glColor3f( 0, 0, 1 );
			for ( const auto &i : world->vertices )
			{
				glVertex3f( i.position.x, i.position.y, i.position.z );
			}
			glEnd();
			glPointSize( 1 );
			break;
		}
		case EDIT_MODE_FACE:
			glPointSize( 2 );
			glColor3f( 0, 0, 1 );
			glBegin( GL_POINTS );
			for ( const auto &i : world->vertices )
			{
				glVertex3f( i.position.x, i.position.y, i.position.z );
			}
			glEnd();
			glPointSize( 1 );
			break;
		case EDIT_MODE_VERTEX:
		{
			glPointSize( 4 );
			glColor3f( 0, 1, 0 );
			glBegin( GL_POINTS );
			for ( const auto &i : world->vertices )
			{
				glVertex3f( i.position.x, i.position.y, i.position.z );
			}

			// Draw current cursor point
			vec3_t pos;
			ToGridPoint( lastX, lastY, pos );
			glColor3f( 1.0f, 1.0f, 0.5f );
			glVertex3f( pos[ 0 ], pos[ 1 ], 0.0f );

			glEnd();
			glPointSize( 1 );

			DrawCursor( pos[ 0 ], pos[ 1 ] );
			break;
		}
		default:
			break;
	}

	if ( g_qeglobals.d_select_mode == sel_vertex )
	{
	}
	glTranslatef( -g_qeglobals.d_select_translate[ 0 ], -g_qeglobals.d_select_translate[ 1 ], -g_qeglobals.d_select_translate[ 2 ] );

	// now draw camera point
	DrawCameraIcon();
}
