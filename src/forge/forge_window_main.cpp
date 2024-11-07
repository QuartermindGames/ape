// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "forge_window_main.h"
#include "forge_about_dialog.h"
#include "forge_model_editor.h"

#include "forge_editor_world.h"
#include "forge_editor_material.h"

#include "common_project.h"
#include "ape/ape_public_model.h"

#include <FXGLCanvas.h>
#include <FXGLVisual.h>
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

	new FXMenuCommand( menuPane, "New Room...\t\tCreate a new room.", forge_cachedIcons[ FORGE_ICON_TYPE_NEW_WORLD ], this, ID_ROOM_NEW );
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

#if 0
	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuTitle( menuBar_, "&Edit", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuTitle( menuBar_, "&View", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuTitle( menuBar_, "&Tools", nullptr, menuPane );
	auto *subMenu = new FXMenuCascade( menuPane, "Cook" );
	auto *subMenuPane = new FXMenuPane( this );
	new FXMenuCommand( subMenuPane, "Convert Model...\t\tConvert a model from another format.", nullptr, this, 0 );
	new FXMenuCommand( subMenuPane, "Import Texture...\t\tImport an existing texture.", nullptr, this, 0 );
	subMenu->setMenu( subMenuPane );
	if ( !isCookAvailable )
	{
		subMenu->disable();
	}
#endif

	// view
	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuCheck( menuPane, "&Console\t\tShow/hide the console.", this, ID_TOGGLE_CONSOLE );
	new FXMenuTitle( menuBar_, "&View", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuCommand( menuPane, "&About\t\tOpen about dialog.", nullptr, this, ID_ABOUT );
	new FXMenuTitle( menuBar_, "&Help", nullptr, menuPane );

	new FXStatusBar( this, LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X );

	mainFrame              = new FXVerticalFrame( this, LAYOUT_FILL );
	auto *verticalSplitter = new FXSplitter( mainFrame, LAYOUT_MIN_HEIGHT | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_VERTICAL );

	_tabBook = new FXTabBook( verticalSplitter, nullptr, 0, LAYOUT_FILL | LAYOUT_RIGHT );
	_tabBook->setHeight( getHeight() - 128 );

	// Add the console at the bottom
	console = new forge::ConsoleFrame( verticalSplitter );
	console->hide();

	getApp()->addTimeout( this, MainWindow::ID_TICK, APE_DEFAULT_TICK_RATE );
}

void forge::MainWindow::create()
{
	FXMainWindow::create();

	show( PLACEMENT_SCREEN );
	maximize();
}

long forge::MainWindow::on_tick( FXObject *, FXSelector, void * )
{
	ape_tick_frame();

	static unsigned int refreshTime = 0;
	if ( refreshTime <= ape_get_num_ticks() )
	{
		com_profiler_update_samples();

		PL_GET_CVAR( "debug/profilerFrequency", profilerFrequency );
		refreshTime += ( profilerFrequency != nullptr ) ? profilerFrequency->i_value : 16;
	}

	getApp()->addTimeout( this, MainWindow::ID_TICK, APE_DEFAULT_TICK_RATE );
	return 0;
}

long forge::MainWindow::on_new_room( FXObject *, FXSelector, void * )
{
	ApeWorld *world = ape_create_world();
	if ( world == nullptr )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create world instance!\nSee logs for details." );
		return FALSE;
	}

	ApeRoom *room = ape_room_create( &world->base, "index" );
	if ( room == nullptr )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create room!\nSee logs for details." );
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
	FXString    filename = FXFileDialog::getOpenFilename( this, "Select a room", FXString( path ) + "/", "*." APE_WORLD_ROOM_EXTENSION );
	if ( filename.empty() )
	{
		return FALSE;
	}

	AcmBranch *root = acm_load_file( filename.text(), "node" );
	if ( root == nullptr )
	{
		return FALSE;
	}

	ApeWorldNode *roomNode = ape_world_node_deserialize( nullptr, root );
	if ( roomNode == nullptr )
	{
		return FALSE;
	}

	if ( roomNode->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Selected file is not a valid room file!" );
		ape_world_node_destroy( roomNode );
		return FALSE;
	}

	ApeWorld *world = ape_create_world();
	ape_world_node_attach( roomNode, APE_WORLD_NODE( world ) );

	auto *editor = ( WorldEditor * ) add_tab( new WorldEditor( _tabBook, PlGetFileName( filename.text() ), world ) );
	editor->update_tree();
	editor->set_active_room( ( ApeRoom * ) roomNode );

	return TRUE;
}

long forge::MainWindow::on_save_room( FX::FXObject *, FX::FXSelector, void * )
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

	ApeMaterial *material = ape_material_cache( filename.text(), APE_CACHE_GROUP_EDITOR, false, false );
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
	auto *aboutDialog = new forge::AboutDialog( this );
	aboutDialog->execute();
	return true;
}

long forge::MainWindow::on_package_project( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getSaveFilename( this, "Select a destination", FXString( forge::cachedPaths[ forge::PATH_PROJECTS ] ) + "/", "*.pkg" );
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

		auto it   = _tabs.begin() + index;
		auto item = *it;
		item->destroy();

		_tabBook->recalc();

		_tabs.erase( it );
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

/**
 * Push a message to the console.
 */
void forge::MainWindow::push_message( int level, const char *msg, const PLColour &colour )
{
	console->push_message( level, msg, colour );
}

FXTabItem *forge::MainWindow::get_active_tab()
{
	try
	{
		int index = _tabBook->getCurrent();
		if ( index < 0 || index >= _tabs.size() )
		{
			throw;
		}

		return *( _tabs.begin() + index );
	}
	catch ( ... )
	{
		return nullptr;
	}
}

FXTabItem *forge::MainWindow::add_tab( FXTabItem *item )
{
	auto tab = _tabs.emplace_back( item );
	tab->create();

	_tabBook->layout();

	closeEditorCommand->enable();

	return tab;
}
