// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"
#include "editor_frame_viewport.h"
#include "editor_face_inspector.h"
#include "editor_frame_console.h"

namespace os::editor
{
	class ModelWindow;

	class MainWindow : public FXMainWindow
	{
		FXDECLARE( MainWindow )

	public:
		MainWindow( FXApp *app );

	protected:
		inline MainWindow() = default;

	public:
		long OnTick( FXObject *, FXSelector, void *ptr );
		long OnNew( FXObject *, FXSelector, void *ptr );
		long OnOpen( FXObject *, FXSelector, void *ptr );
		long OnAbout( FXObject *, FXSelector, void *ptr );
		long OnPackageProject( FXObject *, FXSelector, void * );

		long OpenModel( FXObject *, FXSelector, void * );

		os::editor::Project *GetProject() { return currentProject; }

		enum
		{
			ID_CANVAS = FXMainWindow::ID_LAST,

			ID_PROJECT_NEW,
			ID_PROJECT_OPEN,

			ID_WORLD_NEW,
			ID_WORLD_OPEN,
			ID_WORLD_SAVE,
			ID_WORLD_SAVEAS,
			ID_WORLD_CLOSE,

			ID_MODEL_OPEN,

			ID_COPY,
			ID_PASTE,
			ID_GRID_TOGGLE,
			ID_TOGGLE_EDIT,
			ID_TIMEOUT,
			ID_TICK,
			ID_OPENGL,
			ID_ABOUT,

			ID_PROJECT_PACKAGE,
		};

		void PushMessage( int level, const char *msg, const PLColour &colour );

	private:
		void create() override;

		os::editor::Project *currentProject{ nullptr };

		FXToolBar *toolBar_;
		FXMenuBar *menuBar_;

		FXDataTarget gridHideTarget;
		FXDataTarget gridSizeTarget;

		FXMenuPane *fileMenu_;
		FXMenuPane *editMenu_;
		FXMenuPane *viewMenu_;

		FXVerticalFrame *mainFrame;

		FXToggleButton *editModeButtons[ EDITOR_MAX_GEOMETRYMODES ];

		FXStatusBar *statusBars_[ 3 ];

		FXGLVisual *glVisual_;

		os::editor::ConsoleFrame *consoleFrame;

		EditorViewportFrame *viewportFrame;
		EditorFaceInspector *faceInspectorWindow;

		ModelWindow *modelWindow;
	};

	extern MainWindow *mainWindow;
}// namespace os::editor