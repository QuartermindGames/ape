// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"
#include "ViewportFrame.h"
#include "ConsoleFrame.h"

namespace ss::forge
{
	class MainWindow : public FXMainWindow
	{
		FXDECLARE( MainWindow )

	public:
		explicit MainWindow( FXApp *app );

	protected:
		inline MainWindow() = default;

	public:
		long on_tick( FXObject *, FXSelector, void * );
		long on_new_world( FXObject *, FXSelector, void * );
		long on_open_world( FXObject *, FXSelector, void * );

		long open_model( FXObject *, FXSelector, void * );
		long open_material( FXObject *, FXSelector, void * );

		long on_about( FXObject *, FXSelector, void * );
		long on_package_project( FXObject *, FXSelector, void * );

		void setup_engine_viewports();

		Project *GetProject() { return currentProject; }

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
			ID_MATERIAL_NEW,
			ID_MATERIAL_OPEN,
			ID_TEXTURE_OPEN,

			ID_SETTINGS,

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

		Project *currentProject{ nullptr };

		FXMenuBar *menuBar_{};

		FXVerticalFrame *mainFrame{};

		FXStatusBar *statusBars_[ 3 ]{};

	public:
		static FXGLVisual *glVisual_;

	private:
		ConsoleFrame *consoleFrame{};

	private:
		FXTabBook *_tabBook{};
		std::vector< FXTabItem * > _tabs;
	};

	extern MainWindow *mainWindow;
}// namespace ss::forge