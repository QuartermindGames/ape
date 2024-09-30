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
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_WORLD_NEW, forge::MainWindow::on_new_world ),
        FXMAPFUNC( SEL_COMMAND, forge::MainWindow::ID_WORLD_OPEN, forge::MainWindow::on_open_world ),

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

	new FXMenuCommand( menuPane, "New World\t\tCreate a new world.", forge::load_fx_icon( getApp(), "resources/new_world.gif" ), this, ID_WORLD_NEW );
	new FXMenuCommand( menuPane, "Open World...\t\tOpen an existing world.", forge::load_fx_icon( getApp(), "resources/open_world.gif" ), this, ID_WORLD_OPEN );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "Open Model...\t\tOpen an existing model.", forge::load_fx_icon( getApp(), "resources/open_model.gif" ), this, ID_MODEL_OPEN );
	new FXMenuSeparator( menuPane );

	closeEditorCommand = new FXMenuCommand( menuPane, "Close Editor", forge::load_fx_icon( getApp(), "resources/close.gif" ), this, ID_CLOSE_EDITOR );
	closeEditorCommand->disable();

	new FXMenuSeparator( menuPane );

#if 0
	new FXMenuCommand( menuPane, "Open Model...\t\tOpen an existing model.", forge::load_fx_icon( getApp(), "resources/open_model.gif" ), this, ID_MODEL_OPEN );
	new FXMenuSeparator( menuPane );

	new FXMenuCommand( menuPane, "New Material\t\tCreate a new material.", forge::load_fx_icon( getApp(), "resources/new_material.gif" ), this, ID_MATERIAL_NEW );
	new FXMenuCommand( menuPane, "Open Material...\t\tOpen an existing material.", forge::load_fx_icon( getApp(), "resources/open_material.gif" ), this, ID_MATERIAL_OPEN );
	new FXMenuSeparator( menuPane );

	new FXMenuCommand( menuPane, "Package Project\t\tPackage the current project.", nullptr, this, ID_PROJECT_PACKAGE );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "Settings...\t\tConfigure editor settings and more.", forge::load_fx_icon( getApp(), "resources/wrench.gif" ), this, ID_SETTINGS );
	new FXMenuSeparator( menuPane );
#endif

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

	getApp()->addTimeout( this, MainWindow::ID_TICK, APE_DEFAULT_TICK_RATE );
	return 0;
}

long forge::MainWindow::on_new_world( FXObject *, FXSelector, void * )
{
	ApeWorld *world = ape_create_world();
	if ( world == nullptr )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create world!\nSee logs for details." );
		return FALSE;
	}

	ape_room_create( &world->base, "room" );

	auto *editor = ( WorldEditor * ) add_tab( new WorldEditor( _tabBook, "", world ) );
	editor->update_tree();

	return TRUE;
}

long forge::MainWindow::on_open_world( FXObject *, FXSelector, void * )
{
	const char *path     = com_project_get_local_path();
	FXString    filename = FXFileDialog::getOpenFilename( this, "Select a world", FXString( path ) + "/", "*.wld.n" );
	if ( filename.empty() )
	{
		return FALSE;
	}

	ApeWorld *world = ape_world_load( filename.text() );
	if ( world == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK,
		                       "Warning",
		                       "Failed to open world (%s)!\n"
		                       "See logs for details.",
		                       filename.text() );
		return FALSE;
	}

	auto *editor = ( WorldEditor * ) add_tab( new WorldEditor( _tabBook, PlGetFileName( filename.text() ), world ) );
	editor->update_tree();

	return TRUE;
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
