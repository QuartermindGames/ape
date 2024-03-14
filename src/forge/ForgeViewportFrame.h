// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "editor.h"

namespace ss::forge
{
	class viewport_frame : public FXVerticalFrame
	{
		FXDECLARE( viewport_frame )

	public:
		viewport_frame( FXComposite *composite, FXGLVisual *visual, FXTabItem *editor, ApeCameraViewMode viewMode );
		virtual ~viewport_frame();

		void create() override;

		enum
		{
			ID_DRAW = FXVerticalFrame::ID_LAST,
			ID_CANVAS,
			ID_TOGGLE_VIEW,
			ID_TOGGLE_DRAW,

			ID_PERSPECTIVE,
			ID_TOP,
			ID_LEFT,
			ID_FRONT,

			ID_WIREFRAME,
			ID_SOLID,
			ID_TEXTURED,
			ID_LIT,

			ID_BUTTON_CREATE_ROOM,
			ID_BUTTON_CREATE_BRUSH,
			ID_BUTTON_CREATE_LIGHT,
			ID_BUTTON_CREATE_CAMERA,
			ID_BUTTON_CREATE_ENTITY,

			ID_BUTTON_RESET_CAMERA,

			ID_LAST
		};

		virtual void Draw();

		inline void set_active( bool state )
		{
			_isActive = state;
		}

		[[nodiscard]] bool is_editor_active() const;

	private:
		bool _isActive{ true };

	public:
		long on_change_camera_modes( FXObject *, FXSelector, void * );

		long on_chore( FXObject *, FXSelector, void * );
		long on_zoom( FXObject *, FXSelector, void * );
		long on_motion( FXObject *, FXSelector, void * );
		long on_right_click( FXObject *, FXSelector, void * );
		long on_middle_click( FXObject *, FXSelector, void * );
		long on_key( FXObject *, FXSelector, void * );
		long on_create( FXObject *, FXSelector, void * );
		long on_reset_camera( FXObject *, FXSelector, void * );

	private:
		inline viewport_frame() = default;

		FXGLCanvas *canvas_;

		ApeCameraDrawMode drawMode_{ APE_CAMERA_DRAW_MODE_WIREFRAME };
		ApeCameraViewMode viewMode_{ APE_CAMERA_MODE_INVALID };

		static int translate_key( int code );

	public:
		ApeViewport *internalViewport_{};
		ApeCamera *camera{};
		static unsigned int cameraTagNum;

	private:
		FXDataTarget forwardSpeedTarget_;
		FXDataTarget turnSpeedTarget_;

		static FXGLCanvas *displayList_;

		FXTabItem *editor{};

		bool useMouseLook{};
		FXint originCursorPos[ 2 ];
	};
}// namespace ss::forge