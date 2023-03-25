// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

class EditorViewportFrame : public FXVerticalFrame
{
	FXDECLARE( EditorViewportFrame )

public:
	EditorViewportFrame( FXComposite *composite, FXGLVisual *visual, YNCoreCameraMode viewMode );
	virtual ~EditorViewportFrame();

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
	inline EditorViewportFrame() = default;

	FXToolBar *toolBar_;
	FXGLCanvas *canvas_;
	FXGLVisual *visual_;
	FXGLContext *context_;

	FXToggleButton *viewModeButtons_[ YN_CORE_CAMERA_MAX_MODES ];
	FXToggleButton *drawModeButtons_[ YN_CORE_CAMERA_MAX_DRAW_MODES ];

	YNCoreCameraDrawMode drawMode_{ YN_CORE_CAMERA_DRAW_MODE_WIREFRAME };

	float zoomScale_{ 1.0f };

public:
	YNCoreViewport *engineViewportHandle;

private:
	FXDataTarget forwardSpeedTarget_;
	FXDataTarget turnSpeedTarget_;

	static FXGLCanvas *displayList_;
};
