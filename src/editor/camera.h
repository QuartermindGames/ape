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

// window system independent camera view code

#pragma once

#include "brush.h"

namespace huang {
	class Camera {
	public:
		Camera();

		void BuildMatrix();
		void ChangeFloor( bool up );

		void PositionDrag();
		void MouseControl( float dtime, const bool buttons[] );
		bool MouseDown( int x, int y, const bool buttons[] );
		void MouseUp( int x, int y, const bool buttons[] );
		void MouseMoved( int x, int y, const bool buttons[] );

		bool HandleInput( int key );

		void InitCull();
		bool CullBrush( brush_t *b );

		void Draw();
		void DrawBrush( brush_t *b );

		void ResetPosition();
		void CenterView();

		int		width{ 0 }, height{ 0 };

		qboolean	timing{ false };

		vec3_t	origin{ 0.0f, 20.0f, 46.0f };
		vec3_t	angles{ 0.0f, 0.0f, 0.0f };

		uint8_t draw_mode{ DRAW_TEXTURED };

		vec3_t	color{ 0.3f, 0.3f, 0.3f };			// background

		vec3_t	forward, right, up;	// move matrix

	private:
		vec3_t	vup, vpn, vright;	// view matrix

		vec3_t	cull1, cull2;
		int		cullv1[ 3 ], cullv2[ 3 ];
	};
}
