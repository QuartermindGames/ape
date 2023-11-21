// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

namespace os::editor
{
	class ViewportFrame : public FXVerticalFrame
	{
		FXDECLARE( ViewportFrame )

	public:
		ViewportFrame( FXComposite *composite, FXGLVisual *visual, ApeCameraMode viewMode );
		virtual ~ViewportFrame();

		void create() override;

		enum
		{
			ID_CHORE = FXVerticalFrame::ID_LAST,
			ID_CANVAS,
			ID_TOGGLE_VIEW,
			ID_TOGGLE_DRAW,
			ID_LAST
		};

		virtual void Draw();

		long OnChore( FXObject *, FXSelector, void * );
		long OnMotion( FXObject *, FXSelector, void * );

	private:
		inline ViewportFrame() = default;

		FXToolBar *toolBar_;
		FXGLCanvas *canvas_;
		FXGLVisual *visual_;
		FXGLContext *context_;

		FXToggleButton *viewModeButtons_[ APE_CAMERA_MAX_MODES ];
		FXToggleButton *drawModeButtons_[ APE_CAMERA_MAX_DRAW_MODES ];

		ApeCameraDrawMode drawMode_{ APE_CAMERA_DRAW_MODE_WIREFRAME };

		float zoomScale_{ 1.0f };

	public:
		SS_Arl_Viewport *engineViewportHandle;

	private:
		FXDataTarget forwardSpeedTarget_;
		FXDataTarget turnSpeedTarget_;

		static FXGLCanvas *displayList_;
	};
}// namespace os::editor