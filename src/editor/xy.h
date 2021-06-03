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

#include "BasePerspective.h"

namespace huang
{
	class XYZPerspective : public BasePerspective
	{
	public:
		enum class Angle
		{
			TOP,
			LEFT,
			FRONT
		};

		explicit XYZPerspective( Viewport *parent );
		~XYZPerspective();

		void MouseDown( int x, int y, const bool buttons[] );
		void MouseUp( int x, int y, const bool buttons[] );
		void MouseMoved( int x, int y, const bool buttons[] );

		void Draw();

		float scale{ 1.0f };

	protected:
	private:
		void ToPoint( int x, int y, vec3_t point );
		void ToGridPoint( int x, int y, vec3_t point );

		bool DragDelta( int x, int y, vec3_t move );

		void NewBrushDrag( int x, int y );

		void DrawGrid();
		void DrawBlockGrid();

		bool d_dirty{ false };
	};
}// namespace huang

bool FilterBrush( const Brush *pb );
