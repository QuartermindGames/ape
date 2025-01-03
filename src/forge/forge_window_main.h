// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge.h"
#include "forge_viewport.h"
#include "forge_console_frame.h"
#include "forge_dialog_properties.h"

namespace forge
{
	class MaterialBrowser;
	class MainWindow : public FXMainWindow
	{
		FXDECLARE( MainWindow )

	public:
		explicit MainWindow( FXApp *app );

	protected:
		inline MainWindow() = default;

	public:
		long on_tick( FXObject *, FXSelector, void * );
		long on_new_room( FXObject *, FXSelector, void * );
		long on_open_room( FXObject *, FXSelector, void * );
		long on_save_room( FXObject *, FXSelector, void * );

		long open_model( FXObject *, FXSelector, void * );
		long open_material( FXObject *, FXSelector, void * );

		long on_about( FXObject *, FXSelector, void * );
		long on_package_project( FXObject *, FXSelector, void * );

		long on_close_editor( FXObject *, FXSelector, void * );

		long on_toggle_console( FXObject *, FXSelector, void * );
		long on_toggle_node_volumes( FXObject *, FXSelector, void * );

		void        open_material_browser();
		const char *get_active_material();

		void open_properties( ApeWorldNode *node );

		Project *GetProject() { return currentProject; }

		enum
		{
			ID_CANVAS = FXMainWindow::ID_LAST,

			ID_ROOM_NEW,
			ID_ROOM_OPEN,
			ID_ROOM_SAVE,
			ID_ROOM_SAVE_AS,

			ID_MODEL_OPEN,
			ID_MATERIAL_NEW,
			ID_MATERIAL_OPEN,

			ID_SETTINGS,

			ID_TICK,
			ID_ABOUT,

			ID_TOGGLE_CONSOLE,
			ID_TOGGLE_NODE_VOLUMES,

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

		MaterialBrowser  *materialBrowser{};
		PropertiesDialog *propertiesDialog{};

	public:
		FXTabItem *get_active_tab();
		FXTabItem *add_tab( FXTabItem *item );
	};

	extern MainWindow *mainWindow;
}// namespace forge