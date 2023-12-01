// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"
#include "editor_frame_viewport.h"
#include "editor_face_inspector.h"
#include "editor_frame_console.h"

namespace ss::forge
{
	class ModelWindow;
	class MaterialWindow;

	class MainWindow : public FXMainWindow
	{
		FXDECLARE( MainWindow )

	public:
		explicit MainWindow( FXApp *app );

	protected:
		inline MainWindow() = default;

	public:
		long on_tick( FXObject *, FXSelector, void * );
		long on_new( FXObject *, FXSelector, void * );
		long on_open( FXObject *, FXSelector, void * );

		long open_model( FXObject *, FXSelector, void * );
		long open_texture( FXObject *, FXSelector, void * );
		long open_material( FXObject *, FXSelector, void * );

		long on_about( FXObject *, FXSelector, void * );
		long on_package_project( FXObject *, FXSelector, void * );

		void setup_engine_viewports();

		ss::forge::Project *GetProject() { return currentProject; }

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
			ID_MATERIAL_OPEN,
			ID_TEXTURE_OPEN,

			ID_COPY,
			ID_PASTE,
			ID_GRID_TOGGLE,
			ID_TOGGLE_EDIT,
			ID_TIMEOUT,
			ID_TICK,
			ID_ABOUT,

			ID_PROJECT_PACKAGE,
		};

		void push_message( int level, const char *msg, const PLColour &colour );

	private:
		void create() override;

		ss::forge::Project *currentProject{ nullptr };

		FXToolBar *toolBar_{};
		FXMenuBar *menuBar_{};

		FXDataTarget gridHideTarget;
		FXDataTarget gridSizeTarget;

		FXVerticalFrame *mainFrame{};

		FXToggleButton *editModeButtons[ EDITOR_MAX_GEOMETRYMODES ]{};

		FXStatusBar *statusBars_[ 3 ]{};

		FXGLVisual *glVisual_{};

		ss::forge::ConsoleFrame *consoleFrame{};

		ViewportFrame *viewportFrame[ 4 ]{};

		EditorFaceInspector *faceInspectorWindow{};
		ModelWindow *modelWindow{};
		MaterialWindow *materialWindow{ nullptr };
	};

	extern MainWindow *mainWindow;
}// namespace ss::forge