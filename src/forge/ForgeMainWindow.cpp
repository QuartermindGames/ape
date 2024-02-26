// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ForgeMainWindow.h"
#include "AboutDialog.h"

#include "editors/editor_world.h"
#include "editors/ModelEditor.h"
#include "editors/editor_material.h"

#include "common_project.h"

#include <FXGLCanvas.h>
#include <FXGLVisual.h>

ss::forge::ForgeMainWindow *ss::forge::mainWindow = nullptr;

FXDEFMAP( ss::forge::ForgeMainWindow )
MainWindowMap[] = {
        //FXMAPFUNC( SEL_CONFIGURE, MainWindow::ID_CANVAS, mao::MainWindow::OnConfigure ),
        //FXMAPFUNC( SEL_PAINT, MainWindow::ID_CANVAS, mao::MainWindow::OnExpose ),
        //FXMAPFUNC( SEL_CHORE, MainWindow::ID_TIMEOUT, mao::MainWindow::OnTimeout ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::ForgeMainWindow::ID_WORLD_NEW, ss::forge::ForgeMainWindow::on_new_world ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::ForgeMainWindow::ID_WORLD_OPEN, ss::forge::ForgeMainWindow::on_open_world ),

        FXMAPFUNC( SEL_COMMAND, ss::forge::ForgeMainWindow::ID_MODEL_OPEN, ss::forge::ForgeMainWindow::open_model ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::ForgeMainWindow::ID_MATERIAL_OPEN, ss::forge::ForgeMainWindow::open_material ),

        FXMAPFUNC( SEL_COMMAND, ss::forge::ForgeMainWindow::ID_ABOUT, ss::forge::ForgeMainWindow::on_about ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::ForgeMainWindow::ID_PROJECT_PACKAGE, ss::forge::ForgeMainWindow::on_package_project ),
        //FXMAPFUNC( SEL_COMMAND, MainWindow::ID_TOGGLE_EDIT, mao::MainWindow::OnToggleEdit ),
        //FXMAPFUNC( SEL_KEYRELEASE, MainWindow::ID_CANVAS, mao::MainWindow::OnInput ),
        FXMAPFUNC( SEL_TIMEOUT, ss::forge::ForgeMainWindow::ID_TICK, ss::forge::ForgeMainWindow::on_tick ),
};

FXIMPLEMENT( ss::forge::ForgeMainWindow, FXMainWindow, MainWindowMap, ARRAYNUMBER( MainWindowMap ) )

ss::forge::ForgeMainWindow::ForgeMainWindow( FXApp *app )
    : FXMainWindow( app, FORGE_APP_TITLE, nullptr, nullptr, DECOR_ALL, 0, 0, 1024, 768, 0, 0 )
{
	menuBar_ = new FXMenuBar( this, LAYOUT_SIDE_TOP | LAYOUT_FILL_X );

	auto *menuPane = new FXMenuPane( menuBar_->getParent() );

	new FXMenuCommand( menuPane, "New World\t\tCreate a new world.", ss::forge::load_fx_icon( getApp(), "resources/new_world.gif" ), this, ID_WORLD_NEW );
	new FXMenuCommand( menuPane, "Open World...\t\tOpen an existing world.", ss::forge::load_fx_icon( getApp(), "resources/open_world.gif" ), this, ID_WORLD_OPEN );
	new FXMenuSeparator( menuPane );

#if 0
	new FXMenuCommand( menuPane, "Open Model...\t\tOpen an existing model.", ss::forge::load_fx_icon( getApp(), "resources/open_model.gif" ), this, ID_MODEL_OPEN );
	new FXMenuSeparator( menuPane );

	new FXMenuCommand( menuPane, "New Material\t\tCreate a new material.", ss::forge::load_fx_icon( getApp(), "resources/new_material.gif" ), this, ID_MATERIAL_NEW );
	new FXMenuCommand( menuPane, "Open Material...\t\tOpen an existing material.", ss::forge::load_fx_icon( getApp(), "resources/open_material.gif" ), this, ID_MATERIAL_OPEN );
	new FXMenuSeparator( menuPane );

	new FXMenuCommand( menuPane, "Package Project\t\tPackage the current project.", nullptr, this, ID_PROJECT_PACKAGE );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "Settings...\t\tConfigure editor settings and more.", ss::forge::load_fx_icon( getApp(), "resources/wrench.gif" ), this, ID_SETTINGS );
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

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuCommand( menuPane, "&About\t\tOpen about dialog.", nullptr, this, ID_ABOUT );
	new FXMenuTitle( menuBar_, "&Help", nullptr, menuPane );

	new FXStatusBar( this, LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X );

	mainFrame = new FXVerticalFrame( this, LAYOUT_FILL );
	auto *verticalSplitter = new FXSplitter( mainFrame, LAYOUT_MIN_HEIGHT | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_VERTICAL );

	_tabBook = new FXTabBook( verticalSplitter, nullptr, 0, LAYOUT_FILL_X | LAYOUT_FILL_Y | LAYOUT_RIGHT );
	_tabBook->setHeight( getHeight() - 128 );

	// Add the console at the bottom
	consoleFrame = new ss::forge::ForgeConsoleFrame( verticalSplitter );

	//HACK: make the engine initialisation happy...
	auto *dummy = new viewport_frame( this, get_shared_gl_visual(), nullptr, APE_CAMERA_MODE_PERSPECTIVE );
	//dummy->set_active( false );
	//dummy->hide();

	getApp()->addTimeout( this, ForgeMainWindow::ID_TICK, SS_SHELL_TICK_RATE );
}

void ss::forge::ForgeMainWindow::create()
{
	FXMainWindow::create();

	show( PLACEMENT_SCREEN );
	maximize();
}

long ss::forge::ForgeMainWindow::on_tick( FXObject *, FXSelector, void * )
{
	ape_tick_frame();

	getApp()->addTimeout( this, ForgeMainWindow::ID_TICK, SS_SHELL_TICK_RATE );
	return 0;
}

long ss::forge::ForgeMainWindow::on_new_world( FXObject *, FXSelector, void * )
{
	ApeWorld *world = ape_world_create();
	if ( world == nullptr )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create world!\nSee logs for details." );
		return FALSE;
	}

	auto *editor = new editor_world( _tabBook, "", world );
	editor->update_tree();

	auto tab = _tabs.emplace_back( editor );
	tab->create();

	_tabBook->layout();

	return TRUE;
}

long ss::forge::ForgeMainWindow::on_open_world( FXObject *, FXSelector, void * )
{
	const char *path = com_project_get_local_path();
	FXString filename = FXFileDialog::getOpenFilename( this, "Select a world", FXString( path ) + "/", "*.wld.n" );
	if ( filename.empty() )
		return FALSE;

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

	auto *editor = new editor_world( _tabBook, PlGetFileName( filename.text() ), world );
	editor->update_tree();

	auto tab = _tabs.emplace_back( editor );
	tab->create();

	_tabBook->layout();

	return TRUE;
}

long ss::forge::ForgeMainWindow::open_model( FXObject *, FXSelector, void * )
{
	const char *path = com_project_get_local_path();
	FXString filename = FXFileDialog::getOpenFilename( this, "Select an existing model", FXString( path ) + "/", "*.mdl.n" );
	if ( filename.empty() )
		return false;

	//	auto tab = _tabs.emplace_back( new ModelEditor( _tabBook, PlGetFileName( filename.text() ), world ) );
	//	tab->create();

	_tabBook->layout();

	return true;
}

long ss::forge::ForgeMainWindow::open_material( FXObject *, FXSelector, void * )
{
	const char *path = com_project_get_local_path();
	FXString filename = FXFileDialog::getOpenFilename( this, "Select an existing material", FXString( path ) + "/", "*.mat.n" );
	if ( filename.empty() )
		return false;

	ApeMaterial *material = ss_arl_material_cache( filename.text(), APE_CACHE_EDITOR, false, false );
	if ( material == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to load material (%s)!", filename.text() );
		return false;
	}

	auto tab = _tabs.emplace_back( new editor_material( _tabBook, PlGetFileName( filename.text() ), material ) );
	tab->create();

	_tabBook->layout();

	return true;
}

long ss::forge::ForgeMainWindow::on_about( FXObject *, FXSelector, void * )
{
	auto *aboutDialog = new forge::AboutDialog( this );
	aboutDialog->execute();
	return true;
}

long ss::forge::ForgeMainWindow::on_package_project( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getSaveFilename( this, "Select a destination", FXString( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ] ) + "/", "*.pkg" );
	if ( filename.empty() )
		return false;

	return true;
}

void ss::forge::ForgeMainWindow::setup_engine_viewports()
{
}

/**
 * Push a message to the console.
 */
void ss::forge::ForgeMainWindow::push_message( int level, const char *msg, const PLColour &colour )
{
	consoleFrame->push_message( level, msg, colour );
}
