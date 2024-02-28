// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "editor.h"
#include "ForgeViewportFrame.h"
#include "ForgeConsoleFrame.h"

namespace ss::forge
{
	class ForgeMainWindow : public FXMainWindow
	{
		FXDECLARE( ForgeMainWindow )

	public:
		explicit ForgeMainWindow( FXApp *app );

	protected:
		inline ForgeMainWindow() = default;

	public:
		long on_tick( FXObject *, FXSelector, void * );
		long on_new_world( FXObject *, FXSelector, void * );
		long on_open_world( FXObject *, FXSelector, void * );

		long open_model( FXObject *, FXSelector, void * );
		long open_material( FXObject *, FXSelector, void * );

		long on_about( FXObject *, FXSelector, void * );
		long on_package_project( FXObject *, FXSelector, void * );

		long on_close_editor( FXObject *, FXSelector, void * );

		void setup_engine_viewports();

		Project *GetProject() { return currentProject; }

		enum
		{
			ID_CANVAS = FXMainWindow::ID_LAST,

			ID_WORLD_NEW,
			ID_WORLD_OPEN,

			ID_MODEL_OPEN,
			ID_MATERIAL_NEW,
			ID_MATERIAL_OPEN,

			ID_SETTINGS,

			ID_TICK,
			ID_ABOUT,

			ID_PROJECT_PACKAGE,

			ID_CLOSE_EDITOR,
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
		ForgeConsoleFrame *consoleFrame{};

	private:
		FXTabBook *_tabBook{};
		std::vector< FXTabItem * > _tabs;

		bool isGameRunning{};

	public:
		[[nodiscard]] bool is_game_running() const { return isGameRunning; }

	private:
		FXMenuCommand *closeEditorCommand;
	};

	extern ForgeMainWindow *mainWindow;
}// namespace ss::forge