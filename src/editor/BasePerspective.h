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

#pragma once

namespace huang {
	class BasePerspective {
	public:
		explicit BasePerspective( Viewport *viewport );

		void ResetPosition();

        PLVector3 origin{ 0.0f, 20.0f, 46.0f };

	protected:
		virtual void DrawCameraIcon();
		virtual void DrawCursor( float x, float y );
		virtual void DrawGrid();

		Viewport *parentViewport;

        int dragX{ 0 }, dragY{ 0 };// Drag X and Y
        int lastX{ 0 }, lastY{ 0 };// Last cursor X and Y

	private:

    };
}// namespace huang
