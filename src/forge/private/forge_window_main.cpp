// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "forge_window_main.h"
#include "forge_about_dialog.h"
#include "forge_model_editor.h"
#include "forge_browser_materials.h"

#include "forge_editor_world.h"
#include "forge_editor_material.h"

#include "common_project.h"
#include "ape/ape_public_model.h"

#include <algorithm>

forge::MainWindow *forge::mainWindow = nullptr;

FXDEFMAP( forge::MainWindow )
MainWindowMap[] = {
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_ROOM_NEW, forge::MainWindow::on_new_room ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_ROOM_OPEN, forge::MainWindow::on_open_room ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_ROOM_SAVE, forge::MainWindow::on_save_room ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_ROOM_SAVE_AS, forge::MainWindow::on_save_room ),

        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_MODEL_OPEN, forge::MainWindow::open_model ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_MATERIAL_OPEN, forge::MainWindow::open_material ),

        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_ABOUT, forge::MainWindow::on_about ),

        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_TOGGLE_CONSOLE, forge::MainWindow::on_toggle_console ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_TOGGLE_NODE_VOLUMES, forge::MainWindow::on_toggle_node_volumes ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_TOGGLE_SELECTION_BUFFER, forge::MainWindow::on_toggle_selection_buffer ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_TOGGLE_POST_PROCESSING, forge::MainWindow::on_toggle_post_processing ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_TOGGLE_ROOM_VISIBILITY, forge::MainWindow::on_toggle_room_visibility ),

        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_PROJECT_PACKAGE, forge::MainWindow::on_package_project ),
        FXMAPFUNC( SEL_TIMEOUT, forge::MainWindow::ID_TICK, forge::MainWindow::on_tick ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_CLOSE_EDITOR, forge::MainWindow::on_close_editor ),
};
FXIMPLEMENT( forge::MainWindow, FXMainWindow, MainWindowMap, ARRAYNUMBER( MainWindowMap ) )

forge::MainWindow::MainWindow( FXApp *app )
    : FXMainWindow( app, FORGE_APP_TITLE, nullptr, nullptr, DECOR_ALL, 0, 0, 1024, 768, 0, 0 )
{
	menuBar_ = new FXMenuBar( this, LAYOUT_SIDE_TOP | LAYOUT_FILL_X );

	auto *menuPane = new FXMenuPane( menuBar_->getParent() );

	new FXMenuCommand( menuPane, "New Room...\t\tCreate a new room.", forge_cachedIcons[ FORGE_ICON_TYPE_NEW_ROOM ], this, ID_ROOM_NEW );
	new FXMenuCommand( menuPane, "Open Room...\t\tOpen an existing room.", forge_cachedIcons[ FORGE_ICON_TYPE_OPEN_WORLD ], this, ID_ROOM_OPEN );
	new FXMenuCommand( menuPane, "Save Room\t\tSave the current room.", forge_cachedIcons[ FORGE_ICON_TYPE_SAVE ], this, ID_ROOM_SAVE );
	new FXMenuCommand( menuPane, "Save Room As...\t\tSave the current world to the specified location.", forge_cachedIcons[ FORGE_ICON_TYPE_SAVE ], this, 0 );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "Import Room\t\tImport an existing room into the active world.", nullptr, this, 0 );
	new FXMenuCommand( menuPane, "Export Room\t\tExport the currently active room.", nullptr, this, 0 );
	new FXMenuSeparator( menuPane );

	closeEditorCommand = new FXMenuCommand( menuPane, "Close Editor", forge_cachedIcons[ FORGE_ICON_TYPE_CLOSE ], this, ID_CLOSE_EDITOR );
	closeEditorCommand->disable();

	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "&Quit\t\tQuit the application.", nullptr, this, ID_CLOSE );
	new FXMenuTitle( menuBar_, "&File", nullptr, menuPane );

	// view
	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuCheck( menuPane, "&Console\t\tShow/hide the console.", this, ID_TOGGLE_CONSOLE );
	new FXMenuSeparator( menuPane );
	new FXMenuCheck( menuPane, "&Show Node Volumes\t\tToggle node boundaries.", this, ID_TOGGLE_NODE_VOLUMES );
	new FXMenuCheck( menuPane, "Show Selection Buffer\t\tFor debugging selection buffer.", this, ID_TOGGLE_SELECTION_BUFFER );
	( new FXMenuCheck( menuPane, "Post Processing\t\tEnable/disable post-processing.", this, ID_TOGGLE_POST_PROCESSING ) )->setCheck( true );
	new FXMenuCheck( menuPane, "Show All Rooms\t\tToggles visibility of all rooms.", this, ID_TOGGLE_ROOM_VISIBILITY );

	new FXMenuTitle( menuBar_, "&View", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuCommand( menuPane, "&About\t\tOpen about dialog.", nullptr, this, ID_ABOUT );
	new FXMenuTitle( menuBar_, "&Help", nullptr, menuPane );

	new FXStatusBar( this, LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X );

	mainFrame              = new FXVerticalFrame( this, LAYOUT_FILL );
	auto *verticalSplitter = new FXSplitter( mainFrame, LAYOUT_FILL | SPLITTER_TRACKING | SPLITTER_VERTICAL );

	_tabBook = new FXTabBook( verticalSplitter, nullptr, 0, LAYOUT_FILL | LAYOUT_RIGHT );
	_tabBook->setHeight( getHeight() - 128 );

	// Add the console at the bottom
	console = new ConsoleFrame( verticalSplitter );
	console->hide();

	getApp()->addTimeout( this, ID_TICK, APE_DEFAULT_TICK_RATE );

	autosaveTimeout = AUTOSAVE_DELAY;
}

void forge::MainWindow::create()
{
	FXMainWindow::create();

	show( PLACEMENT_SCREEN );
	maximize();
}

long forge::MainWindow::on_tick( FXObject *, FXSelector, void * )
{
	if ( ape_is_running() )
	{
		ape_tick_frame();

		static unsigned int refreshTime = 0;
		if ( refreshTime <= ape_get_num_ticks() )
		{
			com_profiler_update_samples();

			PL_GET_CVAR( "debug/profilerFrequency", profilerFrequency );
			refreshTime += ( profilerFrequency != nullptr ) ? profilerFrequency->i_value : 16;
		}
	}

	EditorTab *tab = dynamic_cast< EditorTab * >( get_active_tab() );
	if ( tab != nullptr )
	{
		if ( autosaveTimeout <= 0 )
		{
			tab->autosave();
			autosaveTimeout = AUTOSAVE_DELAY;
		}

		autosaveTimeout--;
	}

	getApp()->addTimeout( this, ID_TICK, APE_DEFAULT_TICK_RATE );
	return 0;
}

long forge::MainWindow::on_new_room( FXObject *, FXSelector, void * )
{
	ApeWorld *world = ape_world_create();
	if ( world == nullptr )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create world instance!\nSee logs for details." );
		return FALSE;
	}

	ApeRoom *room = ape_room_create( &world->base, "index" );
	if ( room == nullptr )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create room!\nSee logs for details." );
		ape_world_node_destroy( &world->base );
		return FALSE;
	}

	WorldEditor *editor = ( WorldEditor * ) add_tab( new WorldEditor( _tabBook, "", world ) );
	editor->update_tree();
	editor->set_active_room( room );

	return TRUE;
}

long forge::MainWindow::on_open_room( FXObject *, FXSelector, void * )
{
	const char *path     = com_project_get_local_path();
	FXString    filename = FXFileDialog::getOpenFilename( this, "Select a room", FXString( path ) + "/dev/rooms/", "*." APE_WORLD_ROOM_EXTENSION );
	if ( filename.empty() )
	{
		return FALSE;
	}

	ApeRoom *room = forge_load_room_( filename.text() );
	if ( room == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to open room, check log for details!" );
		return false;
	}

	//TODO: move world creation and attachment into world editor class
	ApeWorld *world = ape_world_create();
	ape_world_node_attach( APE_WORLD_NODE( room ), APE_WORLD_NODE( world ) );

	auto *editor = static_cast< WorldEditor * >( add_tab( new WorldEditor( _tabBook, PlGetFileName( filename.text() ), world ) ) );
	editor->update_tree();
	editor->set_active_room( room );

	return TRUE;
}

long forge::MainWindow::on_save_room( FXObject *, FXSelector, void * )
{
	WorldEditor *editor = dynamic_cast< WorldEditor * >( get_active_tab() );
	if ( editor == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Can't save room for current editor!" );
		return false;
	}

	return editor->on_room_save( nullptr, 0, nullptr );
}

long forge::MainWindow::open_model( FXObject *, FXSelector, void * )
{
	const char *path     = com_project_get_local_path();
	FXString    filename = FXFileDialog::getOpenFilename( this, "Select a model", FXString( path ) + "/", "*.mdl.n" );
	if ( filename.empty() )
	{
		return FALSE;
	}

	ApeModel *model = ape_model_load( filename.text() );
	if ( model == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK,
		                       "Warning",
		                       "Failed to open model (%s)!\n"
		                       "See logs for details.",
		                       filename.text() );
		return FALSE;
	}

	add_tab( new editor_model( _tabBook, PlGetFileName( filename.text() ), model ) );

	return TRUE;
}

long forge::MainWindow::open_material( FXObject *, FXSelector, void * )
{
	const char *path     = com_project_get_local_path();
	FXString    filename = FXFileDialog::getOpenFilename( this, "Select an existing material", FXString( path ) + "/", "*.mat.n" );
	if ( filename.empty() )
	{
		return false;
	}

	ApeMaterial *material = ape_material_cache( filename.text(), APE_CACHE_GROUP_EDITOR, false );
	if ( material == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to load material (%s)!", filename.text() );
		return false;
	}

	add_tab( new MaterialEditor( _tabBook, PlGetFileName( filename.text() ), material ) );

	return true;
}

long forge::MainWindow::on_about( FXObject *, FXSelector, void * )
{
	auto *aboutDialog = new AboutDialog( this );
	aboutDialog->execute();
	return true;
}

long forge::MainWindow::on_package_project( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getSaveFilename( this, "Select a destination", FXString( cachedPaths[ PATH_PROJECTS ] ) + "/", "*.pkg" );
	if ( filename.empty() )
	{
		return false;
	}

	return true;
}

long forge::MainWindow::on_close_editor( FXObject *, FXSelector, void * )
{
	try
	{
		int index = _tabBook->getCurrent();
		if ( index < 0 || index >= _tabs.size() )
		{
			throw;
		}

		auto it = _tabs.begin() + index;
		_tabs.erase( it );

		auto item = *it;
		item->destroy();

		_tabBook->recalc();

		delete item;

		return true;
	}
	catch ( ... )
	{
	}

	if ( _tabs.empty() )
	{
		closeEditorCommand->disable();
	}

	return false;
}

long forge::MainWindow::on_toggle_console( FXObject *object, FXSelector, void * )
{
	assert( console != nullptr );

	auto checkBox = dynamic_cast< FXMenuCheck * >( object );
	if ( checkBox != nullptr && checkBox->getCheck() )
	{
		console->show();
	}
	else
	{
		console->hide();
	}

	console->getParent()->recalc();
	update();
	getApp()->refresh();

	return TRUE;
}

long forge::MainWindow::on_toggle_node_volumes( FXObject *object, FXSelector, void * )
{
	PlSetConsoleVariableByName( "world.showNodeVolumes", static_cast< FXMenuCheck * >( object )->getCheck() ? "true" : "false" );
	return TRUE;
}

long forge::MainWindow::on_toggle_selection_buffer( FXObject *object, FXSelector, void * )
{
	PlSetConsoleVariableByName( "renderer.showSelectionBuffer", static_cast< FXMenuCheck * >( object )->getCheck() ? "true" : "false" );
	return TRUE;
}

long forge::MainWindow::on_toggle_post_processing( FXObject *object, FXSelector, void * )
{
	PlSetConsoleVariableByName( "postfx", static_cast< FXMenuCheck * >( object )->getCheck() ? "true" : "false" );
	return TRUE;
}

long forge::MainWindow::on_toggle_room_visibility( FXObject *object, FXSelector, void * )
{
	PlSetConsoleVariableByName( "world.showAllRooms", static_cast< FXMenuCheck * >( object )->getCheck() ? "true" : "false" );
	return true;
}

void forge::MainWindow::open_material_browser()
{
	if ( materialBrowser == nullptr )
	{
		materialBrowser = new MaterialBrowser( this );
		materialBrowser->create();
	}

	materialBrowser->show();
}

const char *forge::MainWindow::get_active_material()
{
	if ( materialBrowser == nullptr )
	{
		return nullptr;
	}

	return materialBrowser->get_current();
}

void forge::MainWindow::open_properties( ApeWorldNode *node )
{
	if ( propertiesDialog == nullptr )
	{
		propertiesDialog = new PropertiesDialog( this, node );
		propertiesDialog->create();
	}

	propertiesDialog->set_node( node );
	propertiesDialog->show();
}

/**
 * Push a message to the console.
 */
void forge::MainWindow::push_message( int level, const char *msg, const QmMathColour4ub &colour )
{
	console->push_message( level, msg, colour );
}

FXTabItem *forge::MainWindow::get_active_tab()
{
	int index = _tabBook->getCurrent();
	if ( index < 0 || index >= _tabs.size() )
	{
		return nullptr;
	}

	return *( _tabs.begin() + index );
}

FXTabItem *forge::MainWindow::add_tab( FXTabItem *item )
{
	FXTabItem *tab = _tabs.emplace_back( item );
	tab->create();

	_tabBook->layout();

	closeEditorCommand->enable();

	return tab;
}
