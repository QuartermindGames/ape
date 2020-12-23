/**
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
**/

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
		FXMenuRadio *drawModeButtons[ MAX_DRAW_MODES ];

		Viewport() {}

	public:
		Viewport( FXComposite *p, FXGLVisual *visual, ViewMode mode );
		virtual ~Viewport();

		virtual void create();

		long OnChore( FXObject *, FXSelector, void * );
		long OnExpose( FXObject *, FXSelector, void * );
		long OnConfigure( FXObject *, FXSelector, void * );

		// Input
		FX_EVENT_FUNC( OnMotion );
		FX_EVENT_FUNC( OnRightButtonPress );
		FX_EVENT_FUNC( OnRightButtonRelease );
		FX_EVENT_FUNC( OnLeftButtonPress );
		FX_EVENT_FUNC( OnLeftButtonRelease );
		FX_EVENT_FUNC( OnInput );
		FX_EVENT_FUNC( OnToggleView );

		void ResetViews();

		enum {
			ID_CHORE = FXVerticalFrame::ID_LAST,
			ID_CANVAS,

			ID_TOGGLE_VIEW,

			ID_LAST
		};

	private:
		void DrawScene();

		Camera camera;

		bool mouseButtonStates[ input::MAX_MOUSE_BUTTONS ];

		uint8_t currentViewMode{ VIEW_MODE_PERSPECTIVE };
		uint8_t currentDrawMode{ DRAW_WIREFRAME };

		float zoomScale{ 1.0f };

		static FXGLCanvas *sharedDisplayList;
	};
}
