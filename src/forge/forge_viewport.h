// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge.h"
#include "forge_editor_tab.h"

namespace forge
{
	class Viewport : public FXVerticalFrame
	{
		FXDECLARE( Viewport )

	public:
		Viewport( FXComposite *composite, FXGLVisual *visual, EditorTab *editor, ApeCameraViewMode viewMode );
		~Viewport() override;

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

		virtual void draw();

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
		virtual long on_motion( FXObject *, FXSelector, void * );
		virtual long on_left_click( FXObject *, FXSelector, void * );
		virtual long on_right_click( FXObject *, FXSelector, void * );
		long on_middle_click( FXObject *, FXSelector, void * );
		virtual long on_key( FXObject *, FXSelector, void * );
		long on_create( FXObject *, FXSelector, void * );
		long on_reset_camera( FXObject *, FXSelector, void * );

	private:
		FXGLCanvas *canvas_;
		FXToolBar  *toolBar;

		FXToggleButton *viewModeButtons[ APE_CAMERA_MAX_MODES ];
		FXToggleButton *drawModeButtons[ APE_CAMERA_MAX_DRAW_MODES ];

		ApeCameraDrawMode drawMode_{ APE_CAMERA_DRAW_MODE_WIREFRAME };
		ApeCameraViewMode viewMode_{ APE_CAMERA_MODE_INVALID };

		static int translate_key( int code );

	protected:
		inline Viewport() = default;

		EditorTab *editor{};

	public:
		ApeViewport        *internalViewport_{};
		ApeCamera          *camera{};

	private:
		FXDataTarget forwardSpeedTarget;
		FXDataTarget turnSpeedTarget;

		static FXGLCanvas *displayList_;

		bool  useMouseLook{};
		FXint originCursorPos[ 2 ];
	};
}// namespace forge