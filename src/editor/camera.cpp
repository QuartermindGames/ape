/*
===========================================================================
Copyright (C) 1997-2006 Id Software, Inc.
Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com>

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

#define	PAGEFLIPS	2

void DrawPathLines (void);

huang::Camera::Camera() {
	VectorCopy( vec3_origin, forward );
	VectorCopy( vec3_origin, right );
	VectorCopy( vec3_origin, up );

	VectorCopy( vec3_origin, vup );
	VectorCopy( vec3_origin, vpn );
	VectorCopy( vec3_origin, vright );

	VectorCopy( vec3_origin, cull1 );
	VectorCopy( vec3_origin, cull2 );

	cullv1[ 0 ] = cullv1[ 1 ] = cullv1[ 2 ] = 0;
	cullv2[ 0 ] = cullv2[ 1 ] = cullv2[ 2 ] = 0;
}

void huang::Camera::BuildMatrix() {
	float	xa, ya;
	float	matrix[ 4 ][ 4 ];
	int		i;

	xa = angles[ 0 ] / 180 * Q_PI;
	ya = angles[ 1 ] / 180 * Q_PI;

	// the movement matrix is kept 2d

	forward[ 0 ] = cos( ya );
	forward[ 1 ] = sin( ya );
	right[ 0 ] = forward[ 1 ];
	right[ 1 ] = -forward[ 0 ];

	glGetFloatv( GL_PROJECTION_MATRIX, &matrix[ 0 ][ 0 ] );

	for( i = 0; i < 3; i++ ) {
		vright[ i ] = matrix[ i ][ 0 ];
		vup[ i ] = matrix[ i ][ 1 ];
		vpn[ i ] = matrix[ i ][ 2 ];
	}

	VectorNormalize( vright );
	VectorNormalize( vup );
	VectorNormalize( vpn );
}

void huang::Camera::ChangeFloor( bool up ) {
	brush_t *b;
	float	d, bestd, current;
	vec3_t	start, dir;

	start[ 0 ] = origin[ 0 ];
	start[ 1 ] = origin[ 1 ];
	start[ 2 ] = 8192.0f;
	dir[ 0 ] = dir[ 1 ] = 0;
	dir[ 2 ] = -1;

	current = 8192.0f - ( origin[ 2 ] - 48.0f );
	if( up )
		bestd = 0;
	else
		bestd = 16384.0f;

	for( b = active_brushes.next; b != &active_brushes; b = b->next ) {
		if( !Brush_Ray( start, dir, b, &d ) )
			continue;
		if( up && d < current && d > bestd )
			bestd = d;
		if( !up && d > current && d < bestd )
			bestd = d;
	}

	if( bestd == 0 || bestd == 16384 )
		return;

	origin[ 2 ] += current - bestd;
}


//===============================================

static	int	buttonx, buttony;
static	int	cursorx, cursory;

face_t	*side_select;

#define	ANGLE_SPEED	300
#define	MOVE_SPEED	400

void huang::Camera::PositionDrag() {
	int		x, y;

	Sys_GetCursorPos( &x, &y );
	if( x != cursorx || y != cursory ) {
		x -= cursorx;
		VectorMA( origin, x, vright, origin );
		y -= cursory;
		origin[ 2 ] -= y;

		Sys_SetCursorPos( cursorx, cursory );
	}
}

void huang::Camera::MouseControl( float dtime, const bool buttons[] ) {
	int		xl, xh;
	int		yl, yh;
	float	xf, yf;

	if( !buttons[ input::MOUSE_BUTTON_RIGHT ] ) {
		return;
	}

	xf = (float)( buttonx - width / 2 ) / ( width / 2 );
	yf = (float)( buttony - height / 2 ) / ( height / 2 );

	xl = width / 3;
	xh = xl * 2;
	yl = height / 3;
	yh = yl * 2;

#if 0
	// strafe
	if( buttony < yl && ( buttonx < xl || buttonx > xh ) )
		VectorMA( camera.origin, xf * dtime * MOVE_SPEED, camera.right, camera.origin );
	else
#endif
	{
		xf *= 1.0f - fabsf( yf );
		if( xf < 0 ) {
			xf += 0.1f;
			if( xf > 0 )
				xf = 0;
		} else {
			xf -= 0.1f;
			if( xf < 0 )
				xf = 0;
		}

		VectorMA( origin, yf * dtime * MOVE_SPEED, forward, origin );
		angles[ YAW ] += xf * -dtime * ANGLE_SPEED;
	}
}

bool huang::Camera::MouseDown( int x, int y, const bool buttons[] ) {
	buttonx = x;
	buttony = y;

	// LBUTTON = manipulate selection
	// shift-LBUTTON = select
	// middle button = grab texture
	// ctrl-middle button = set entire brush to texture
	// ctrl-shift-middle button = set single face to texture
	if( buttons[ input::MOUSE_BUTTON_LEFT ]
		|| ( buttons[ input::MOUSE_BUTTON_LEFT ] && buttons[ input::BUTTON_MOD_SHIFT ] )
		|| ( buttons[ input::MOUSE_BUTTON_LEFT ] && buttons[ input::BUTTON_MOD_CONTROL ] )
		|| ( buttons[ input::MOUSE_BUTTON_LEFT ] && buttons[ input::BUTTON_MOD_CONTROL ] && buttons[ input::BUTTON_MOD_SHIFT ] )
		|| buttons[ input::MOUSE_BUTTON_MIDDLE ]
		|| ( buttons[ input::MOUSE_BUTTON_MIDDLE ] && buttons[ input::BUTTON_MOD_CONTROL ] )
		|| ( buttons[ input::MOUSE_BUTTON_MIDDLE ] && buttons[ input::BUTTON_MOD_SHIFT ] && buttons[ input::BUTTON_MOD_CONTROL ] ) ) {
		// calc ray direction
		float u = (float)( y - height / 2 ) / ( width / 2 );
		float r = (float)( x - width / 2 ) / ( width / 2 );
		float f = 1.0f;
		vec3_t dir;
		for( int i = 0; i < 3; i++ ) {
			dir[ i ] = vpn[ i ] * f + vright[ i ] * r + vup[ i ] * u;
		}
		VectorNormalize( dir );

		Drag_Begin( x, y, buttons, vright, vup, origin, dir );
		return true;
	}
	
	if( buttons[ input::MOUSE_BUTTON_RIGHT ] ) {
		MouseControl( 0.1f, buttons );
		return true;
	}

	return false;
}

void huang::Camera::MouseUp( int x, int y, const bool buttons[] ) {
	Drag_MouseUp();
}

void huang::Camera::MouseMoved( int x, int y, const bool buttons[] ) {
	buttonx = x;
	buttony = y;

	if( buttons[ input::MOUSE_BUTTON_RIGHT ] && buttons[ input::BUTTON_MOD_CONTROL ] ) {
		PositionDrag();
		return;
	}

	if( buttons[ input::MOUSE_BUTTON_MIDDLE ] && buttons[ input::MOUSE_BUTTON_LEFT ] ) {
		Drag_MouseMoved( x, y, buttons );
	}
}

bool huang::Camera::HandleInput( int key ) {
	switch( key ) {
	case KEY_Up:
		VectorMA( origin, forwardSpeed, forward, origin );
		return true;
	case KEY_Down:
		VectorMA( origin, -forwardSpeed, forward, origin );
		return true;
	case KEY_Left:
		angles[ 1 ] += turnSpeed;
		return true;
	case KEY_Right:
		angles[ 1 ] -= turnSpeed;
		return true;
	case KEY_d:
		origin[ 2 ] += forwardSpeed;
		return true;
	case KEY_c:
		origin[ 2 ] -= forwardSpeed;
		return true;
	case KEY_a:
		angles[ 0 ] += turnSpeed;
		if( angles[ 0 ] > 85.0f ) {
			angles[ 0 ] = 85.0f;
		}
		return true;
	case KEY_z:
		angles[ 0 ] -= turnSpeed;
		if( angles[ 0 ] < -85.0f ) {
			angles[ 0 ] = -85.0f;
		}
		return true;
	case KEY_comma:
		VectorMA( origin, -forwardSpeed, right, origin );
		return true;
	case KEY_period:
		VectorMA( origin, forwardSpeed, right, origin );
		return true;
	}

	return false;
}

void huang::Camera::InitCull() {
	VectorSubtract( vpn, vright, cull1 );
	VectorAdd( vpn, vright, cull2 );

	for( int i = 0; i < 3; i++ ) {
		if( cull1[ i ] > 0 )
			cullv1[ i ] = 3 + i;
		else
			cullv1[ i ] = i;
		if( cull2[ i ] > 0 )
			cullv2[ i ] = 3 + i;
		else
			cullv2[ i ] = i;
	}
}

bool huang::Camera::CullBrush( brush_t *b ) {
	int		i;
	vec3_t	point;
	float	d;

	for( i = 0; i < 3; i++ )
		point[ i ] = b->mins[ cullv1[ i ] ] - origin[ i ];

	d = DotProduct( point, cull1 );
	if( d < -1 )
		return true;

	for( i = 0; i < 3; i++ )
		point[ i ] = b->mins[ cullv2[ i ] ] - origin[ i ];

	d = DotProduct( point, cull2 );
	if( d < -1 )
		return true;

	return false;
}

void huang::Camera::Draw() {
	brush_t *brush;
	face_t *face;
	float	screenaspect;
	float	yfov;
	double	start, end;

	if( !active_brushes.next )
		return;	// not valid yet

	if( timing )
		start = Sys_DoubleTime();

	//
	// clear
	//
	QE_CheckOpenGLForErrors();

	glClearColor(
		g_qeglobals.d_savedinfo.colors[ COLOR_CAMERABACK ][ 0 ],
		g_qeglobals.d_savedinfo.colors[ COLOR_CAMERABACK ][ 1 ],
		g_qeglobals.d_savedinfo.colors[ COLOR_CAMERABACK ][ 2 ],
		0 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	//
	// set up viewpoint
	//
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	screenaspect = (float)width / height;
	yfov = 2 * atan( (float)height / width ) * 180 / Q_PI;
	gluPerspective( yfov, screenaspect, 2, 8192 );

	glRotatef( -90, 1, 0, 0 );	    // put Z going up
	glRotatef( 90, 0, 0, 1 );	    // put Z going up
	glRotatef( angles[ 0 ], 0, 1, 0 );
	glRotatef( -angles[ 1 ], 0, 0, 1 );
	glTranslatef( -origin[ 0 ], -origin[ 1 ], -origin[ 2 ] );

	BuildMatrix();

	InitCull();

	//
	// draw stuff
	//

	switch( draw_mode ) {
	case DRAW_WIREFRAME:
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_TEXTURE_1D );
		glDisable( GL_BLEND );
		glDisable( GL_DEPTH_TEST );
		glColor3f( 1.0, 1.0, 1.0 );
		//		glEnable (GL_LINE_SMOOTH);
		break;

	case DRAW_SOLID:
		glCullFace( GL_FRONT );
		glEnable( GL_CULL_FACE );
		glShadeModel( GL_FLAT );

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glDisable( GL_TEXTURE_2D );

		glDisable( GL_BLEND );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		break;

	case DRAW_TEXTURED:
		glCullFace( GL_FRONT );
		glEnable( GL_CULL_FACE );

		glShadeModel( GL_FLAT );

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glEnable( GL_TEXTURE_2D );

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glDisable( GL_BLEND );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );

#if 0

		{
			GLfloat fogColor[ 4 ] = { 0.0, 1.0, 0.0, 0.25 };

			glFogi( GL_FOG_MODE, GL_LINEAR );
			glHint( GL_FOG_HINT, GL_NICEST );  /*  per pixel   */
			glFogf( GL_FOG_START, -8192 );
			glFogf( GL_FOG_END, 65536 );
			glFogfv( GL_FOG_COLOR, fogColor );

		}

#endif
		break;
#if 0
	case DRAW_BLEND:
		glCullFace( GL_FRONT );
		glEnable( GL_CULL_FACE );

		glShadeModel( GL_FLAT );

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glEnable( GL_TEXTURE_2D );

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		break;
#endif
	}

	glMatrixMode( GL_TEXTURE );
	for( brush = active_brushes.next; brush != &active_brushes; brush = brush->next ) {
		if( CullBrush( brush ) )
			continue;
		if( FilterBrush( brush ) )
			continue;

		DrawBrush( brush );
	}
	glMatrixMode( GL_PROJECTION );

	//
	// now draw selected brushes
	//

	glTranslatef( g_qeglobals.d_select_translate[ 0 ], g_qeglobals.d_select_translate[ 1 ], g_qeglobals.d_select_translate[ 2 ] );
	glMatrixMode( GL_TEXTURE );

	// draw normally
	for( brush = selected_brushes.next; brush != &selected_brushes; brush = brush->next ) {
		DrawBrush( brush );
	}

	// blend on top
	glMatrixMode( GL_PROJECTION );

	glColor4f( 1.0f, 0.0f, 0.0f, 0.3f );
	glEnable( GL_BLEND );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_TEXTURE_2D );
	for( brush = selected_brushes.next; brush != &selected_brushes; brush = brush->next )
		for( face = brush->brush_faces; face; face = face->next )
			Face_Draw( face );
	if( selected_face )
		Face_Draw( selected_face );

	// non-zbuffered outline

	glDisable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glColor3f( 1, 1, 1 );
	for( brush = selected_brushes.next; brush != &selected_brushes; brush = brush->next )
		for( face = brush->brush_faces; face; face = face->next )
			Face_Draw( face );

	// edge / vertex flags

	/*
	if( g_qeglobals.d_select_mode == sel_vertex ) {
		glPointSize( 4 );
		glColor3f( 0, 1, 0 );
		glBegin( GL_POINTS );
		for( i = 0; i < g_qeglobals.d_numpoints; i++ )
			glVertex3fv( g_qeglobals.d_points[ i ] );
		glEnd();
		glPointSize( 1 );
	} else 
	if( g_qeglobals.d_select_mode == sel_edge ) {
		float *v1, *v2;

		glPointSize( 4 );
		glColor3f( 0, 0, 1 );
		glBegin( GL_POINTS );
		for( i = 0; i < g_qeglobals.d_numedges; i++ ) {
			v1 = g_qeglobals.d_points[ g_qeglobals.d_edges[ i ].p1 ];
			v2 = g_qeglobals.d_points[ g_qeglobals.d_edges[ i ].p2 ];
			glVertex3f( ( v1[ 0 ] + v2[ 0 ] ) * 0.5, ( v1[ 1 ] + v2[ 1 ] ) * 0.5, ( v1[ 2 ] + v2[ 2 ] ) * 0.5 );
		}
		glEnd();
		glPointSize( 1 );
	}*/

	//
	// draw pointfile
	//
	glEnable( GL_DEPTH_TEST );

	DrawPathLines();

	if( g_qeglobals.d_pointfile_display_list ) {
		Pointfile_Draw();
		//		glCallList (g_qeglobals.d_pointfile_display_list);
	}

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture( GL_TEXTURE_2D, 0 );

	//	Sys_EndWait();
	if( timing ) {
		end = Sys_DoubleTime();
		Sys_Printf( "Camera: %i ms\n", (int)( 1000 * ( end - start ) ) );
	}
}

void huang::Camera::DrawBrush( brush_t *b ) {
	face_t *face;
	int				i, order;
	qtexture_t *prev = 0;
	winding_t *w;

	if( b->owner->eclass->fixedsize && draw_mode == huang::DRAW_TEXTURED )
		glDisable( GL_TEXTURE_2D );

	// guarantee the texture will be set first
	prev = NULL;
	for( face = b->brush_faces, order = 0; face; face = face->next, order++ ) {
		w = face->face_winding;
		if( !w )
			continue;		// freed face

		if( face->d_texture != prev && draw_mode == huang::DRAW_TEXTURED ) {
			// set the texture for this face
			prev = face->d_texture;
			glBindTexture( GL_TEXTURE_2D, face->d_texture->texture_number );
		}

		if( draw_mode == huang::DRAW_WIREFRAME ) {
			glColor3fv( g_qeglobals.d_savedinfo.colors[ COLOR_CAMERA_WIREFRAME ] );
		} else {
			glColor3fv( face->d_color );
		}

		// draw the polygon
		glBegin( GL_POLYGON );
		for( i = 0; i < w->numpoints; i++ ) {
			if( draw_mode == huang::DRAW_TEXTURED ) {
				glTexCoord2fv( &w->points[ i ][ 3 ] );
			}
			glVertex3fv( w->points[ i ] );
		}
		glEnd();
	}

	if( g_qeglobals.d_select_mode == sel_vertex ) {
		glPointSize( 4 );
		glBegin( GL_POINTS );
		glColor3f( 0.0f, 1.0f, 0.0f );
		for( face = b->brush_faces, order = 0; face; face = face->next, order++ ) {
			w = face->face_winding;
			if( !w ) {
				continue;		// freed face
			}

			for( i = 0; i < w->numpoints; i++ ) {
				glVertex3fv( w->points[ i ] );
			}
		}
		glEnd();
		glPointSize( 1 );
	} else if( g_qeglobals.d_select_mode == sel_edge ) {
		float *v1, *v2;
		glPointSize( 4 );
		glColor3f( 0, 0, 1 );
		glBegin( GL_POINTS );
		for( i = 0; i < g_qeglobals.d_numedges; i++ ) {
			v1 = g_qeglobals.d_points[ g_qeglobals.d_edges[ i ].p1 ];
			v2 = g_qeglobals.d_points[ g_qeglobals.d_edges[ i ].p2 ];
			glVertex3f( ( v1[ 0 ] + v2[ 0 ] ) * 0.5, ( v1[ 1 ] + v2[ 1 ] ) * 0.5, ( v1[ 2 ] + v2[ 2 ] ) * 0.5 );
		}
		glEnd();
		glPointSize( 1 );
	}

	if( b->owner->eclass->fixedsize && draw_mode == huang::DRAW_TEXTURED )
		glEnable( GL_TEXTURE_2D );

	glBindTexture( GL_TEXTURE_2D, 0 );
}

void huang::Camera::ResetPosition() {
	// See if an entity exists that we can reset to
	entity_t *entity = Map_FindClass( "info_player_start" );
	if( entity == nullptr ) {
		entity = Map_FindClass( "info_player_deathmatch" );
		if( entity == nullptr ) {
			VectorCopy( vec3_origin, angles );
			VectorCopy( vec3_origin, origin );
			origin[ 2 ] = 48.0f;
			return;
		}
	}

	GetVectorForKey( entity, "origin", origin );
	angles[ YAW ] = FloatForKey( entity, "angle" );
}

void huang::Camera::CenterView() {
	angles[ ROLL ] = angles[ PITCH ] = 0.0f;
	angles[ YAW ] = 22.5f * floorf( ( angles[ YAW ] + 11.0f ) / 22.5f );
}
