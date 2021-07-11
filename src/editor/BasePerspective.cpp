/*
Yin World Editor
Copyright (C) 2020-2021 OldTimes Software

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "qe3.h"
#include "BasePerspective.h"
#include "CameraPerspective.h"
#include "Viewport.h"

huang::BasePerspective::BasePerspective( Viewport *viewport ) : parentViewport( viewport ) {
}

void huang::BasePerspective::ResetPosition() {
    origin = pl_vecOrigin3;
}

void huang::BasePerspective::DrawCameraIcon() {
	CameraPerspective *cameraPerspective = parentViewport->GetCamera();
	if ( cameraPerspective == nullptr ) {
		return;
	}

    float x = cameraPerspective->origin[ 0 ];
    float y = cameraPerspective->origin[ 1 ];
	float z = cameraPerspective->origin[ 2 ];

	float pitch = cameraPerspective->angles[ PITCH ];
    float yaw = cameraPerspective->angles[ YAW ] / 180.0f * Q_PIF;

	// Draw a box representing the position of the camera
	glColor3f( 1.0f, 1.0f, 1.0f );
	glBegin( GL_LINE_STRIP );
    glVertex3f( x - 8.0f, y - 8.0f, z );
    glVertex3f( x - 8.0f, y + 8.0f, z );
    glVertex3f( x + 8.0f, y + 8.0f, z );
    glVertex3f( x + 8.0f, y - 8.0f, z );
    glVertex3f( x - 8.0f, y - 8.0f, z );
	glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( x + 48.0f * cosf( yaw + Q_PIF / 4 ), y + 48.0f * sinf( yaw + Q_PIF / 4.0f ), 0.0f );
    glVertex3f( x, y, 0 );
    glVertex3f( x + 48.0f * cosf( yaw - Q_PIF / 4 ), y + 48.0f * sinf( yaw - Q_PIF / 4.0f ), 0.0f );
    glEnd();
}

void huang::BasePerspective::DrawCursor( float x, float y ) {
    glBegin( GL_LINES );
    glVertex2f( x + -8.0f, y + -8.0f );
    glVertex2f( x + 8.0f, y + 8.0f );
    glVertex2f( x + 8.0f, y + -8.0f );
    glVertex2f( x + -8.0f, y + 8.0f );
    glEnd();
}

void huang::BasePerspective::DrawGrid() {

}
