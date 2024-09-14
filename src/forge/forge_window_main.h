// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge.h"
#include "forge_viewport.h"
#include "forge_console_frame.h"

namespace forge
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

		long on_close_editor( FXObject *, FXSelector, void * );

		long on_toggle_console( FXObject *, FXSelector, void * );

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

			ID_TOGGLE_CONSOLE,

			ID_PROJECT_PACKAGE,

			ID_CLOSE_EDITOR,
		};

		void push_message( int level, const char *msg, const PLColour &colour );

	private:
		void create() override;

		Project *currentProject{ nullptr };

		FXMenuBar       *menuBar_{};
		FXVerticalFrame *mainFrame{};
		FXStatusBar     *statusBars_[ 3 ]{};

	public:
		static FXGLVisual *glVisual_;

	private:
		ConsoleFrame *console{};

	private:
		FXTabBook                 *_tabBook{};
		std::vector< FXTabItem * > _tabs;

		bool isGameRunning{};

	public:
		[[nodiscard]] bool is_game_running() const { return isGameRunning; }

	private:
		FXMenuCommand *closeEditorCommand;

	public:
		FXTabItem *get_active_tab();
		FXTabItem *add_tab( FXTabItem *item );
	};

	extern MainWindow *mainWindow;
}// namespace forge