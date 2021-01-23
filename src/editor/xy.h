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

namespace huang {
	class XYZView {
	public:
		enum class Angle {
			TOP, LEFT, FRONT
		};

		XYZView();
		~XYZView();

		void MouseDown( int x, int y, const bool buttons[] );
		void MouseUp( int x, int y, const bool buttons[] );
		void MouseMoved( int x, int y, const bool buttons[] );

		void Draw( const huang::Viewport *viewport );
		void Overlay();

		void ResetPosition();

		vec3_t origin;

		int width{ 640 }, height{ 480 };
		
		float scale{ 1.0f };

	protected:
	private:
		void ToPoint( int x, int y, vec3_t point );
		void ToGridPoint( int x, int y, vec3_t point );

		bool DragDelta( int x, int y, vec3_t move );

		void NewBrushDrag( int x, int y );

		void DrawGrid();
		void DrawBlockGrid();

		bool timing{ false };

		float topClip{ 0.0f }, bottomClip{ 0.0f };

		bool d_dirty{ false };
	};
}

bool FilterBrush( const Brush *pb );
