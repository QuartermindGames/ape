/**********************************************************
	Huang, level editor for the Yin Game Engine.
	Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**********************************************************/

#pragma once

#include <fx3d.h>

namespace huang {
	class Viewport : public FXVerticalFrame {
		FXDECLARE( Viewport )

	protected:
		FXToolBar *toolBar{ nullptr };

		FXGLCanvas *glCanvas{ nullptr };
		FXGLVisual *glVisual{ nullptr };

		FXToggleButton *viewModeButtons[ MAX_VIEW_MODES ];
		FXToggleButton *drawModeButtons[ MAX_DRAW_MODES ];

		Viewport() {}

	public:
		Viewport( FXComposite *p, FXGLVisual *visual, ViewMode mode );
		virtual ~Viewport();

		virtual void create();

		long OnChore( FXObject *, FXSelector, void * );
		long OnExpose( FXObject *, FXSelector, void * );
		long OnConfigure( FXObject *, FXSelector, void * );

		const Camera *GetCamera() const { return &camera; }

		uint8_t GetViewMode() const { return currentViewMode; }
		uint8_t GetDrawMode() const { return currentDrawMode; }

		void CentreViewOnBrush( const Brush *brush );

		// Input
		FX_EVENT_FUNC( OnMotion );
		FX_EVENT_FUNC( OnRightButtonPress );
		FX_EVENT_FUNC( OnRightButtonRelease );
		FX_EVENT_FUNC( OnLeftButtonPress );
		FX_EVENT_FUNC( OnLeftButtonRelease );
		FX_EVENT_FUNC( OnInput );
		FX_EVENT_FUNC( OnToggleView );
		FX_EVENT_FUNC( OnToggleDraw );

		void ResetViews();

		enum {
			ID_CHORE = FXVerticalFrame::ID_LAST,
			ID_CANVAS,

			ID_TOGGLE_VIEW,
			ID_TOGGLE_DRAW,

			ID_LAST
		};

	private:
		void DrawScene();

		FXDataTarget myForwardSpeedTarget;
		FXDataTarget myTurnSpeedTarget;

		Camera		camera;
		XYZView		xyz;

		bool mouseButtonStates[ input::MAX_MOUSE_BUTTONS ];

		uint8_t currentViewMode{ VIEW_MODE_PERSPECTIVE };
		uint8_t currentDrawMode{ DRAW_WIREFRAME };

		float zoomScale{ 1.0f };

		static FXGLCanvas *sharedDisplayList;
	};
}
